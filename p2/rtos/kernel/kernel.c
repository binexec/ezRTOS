#include "kernel.h"

#include <stdio.h>
#include <string.h>

volatile unsigned int Tick_Count;				//Number of timer ticks missed


/*Variables accessible by OS*/
volatile PD* Current_Process;					//Process descriptor for the last running process before entering the kernel
volatile unsigned char *KernelSp;				//Pointer to the Kernel's own stack location.
volatile unsigned char *CurrentSp;				//Pointer to the stack location of the current running task. Used for saving into PD during ctxswitch.						//The process descriptor of the currently RUNNING task. CP is used to pass information from OS calls to the kernel telling it what to do.
volatile unsigned int KernelActive;				//Indicates if kernel has been initialzied by OS_Start().
volatile ERROR_TYPE err;						//Error code for the previous kernel operation (if any)


extern volatile PD Process[MAXTHREAD];			//List of all Process Descriptors; declared in task/task.c


/************************************************************************/
/*						  KERNEL-ONLY HELPERS                           */
/************************************************************************/

/*Returns the pointer of a process descriptor in the global process list, by searching for its PID*/
PD* findProcessByPID(int pid)
{
	int i;
	
	//Valid PIDs must be greater than 0.
	if(pid <=0)
	return NULL;
	
	for(i=0; i<MAXTHREAD; i++)
	{
		if (Process[i].pid == pid)
		return &(Process[i]);
	}
	
	//No process with such PID
	return NULL;
}


static void print_processes()
{
	int i;
	
	for(i=0; i<MAXTHREAD; i++)
	{
		if(Process[i].state != DEAD)
			printf("\tPID: %d\t State: %d\t Priority: %d\n", Process[i].pid, Process[i].state, Process[i].pri);
	}
	printf("\n");
}

/************************************************************************/
/*				   		       OS HELPERS                               */
/************************************************************************/

/*Returns the PID associated with a function's memory address*/
int findPIDByFuncPtr(voidfuncptr f)
{
	int i;
	
	for(i=0; i<MAXTHREAD; i++)
	{
		if (Process[i].code == f)
			return Process[i].pid;
	}
	
	//No process with such PID
	return -1;
}


/************************************************************************/
/*				   		       TIMER                             */
/************************************************************************/

void Kernel_Tick_Handler()
{
	//No new ticks has been issued yet, skipping...
	if(Tick_Count == 0)
		return;
	
	Task_Tick_Handler();
	Tick_Count = 0;
}

//This function is called ONLY by the timer ISR. Therefore, it will not enter the kernel through regular modes
void Kernel_Tick_ISR()
{
	//Automatically invoke the tick handler if it hasn't been serviced in a long time
	if(++Tick_Count >=  MAX_TICK_MISSED)
	{
		Disable_Interrupt();
		Kernel_Tick_Handler();
		Enable_Interrupt();
	}
	
	#ifdef PREEMPTIVE_CSWITCH
	Scheduler_Tick_ISR();
	#endif
}



/**
  * This internal kernel function is the "main" driving loop of this full-served
  * model architecture. Basically, on OS_Start(), the kernel repeatedly
  * requests the next user task's next system call and then invokes the
  * corresponding kernel function on its behalf.
  *
  * This is the main loop of our kernel, called by OS_Start().
  */
static void Kernel_Handle_Request() 
{
	Kernel_Dispatch_Next_Task();	//Select an initial task to run

	//After OS initialization, THIS WILL BE KERNEL'S MAIN LOOP!
	//NOTE: When another task makes a syscall and enters the loop, it's still in the RUNNING state!
	while(1) 
	{
		//Clears the process' request fields
		Current_Process->request = NONE;
		//Cp->request_arg is not reset, because task_sleep uses it to keep track of remaining ticks

		//Load the current task's stack pointer and switch to its context
		CurrentSp = Current_Process->sp;
		Exit_Kernel();


		/************************************************************************/
		/*     if this task makes a system call, it will return to here!		*/
		/************************************************************************/
		
		
		//Save the current task's stack pointer and proceed to handle its request
		Current_Process->sp = CurrentSp;
		
		//Check if any timer ticks came in
		Kernel_Tick_Handler();

		switch(Current_Process->request)
		{
			case CREATE_T:
			Kernel_Create_Task(Current_Process->code, Current_Process->pri, Current_Process->arg);
			break;
			
			case TERMINATE:
			Kernel_Terminate_Task();
			Kernel_Dispatch_Next_Task();					//Dispatch is only needed if the syscall requires running a different task  after it's done
			break;
		   
			case SUSPEND:
			Kernel_Suspend_Task();
			break;
			
			case RESUME:
			Kernel_Resume_Task();
			break;
			
			case SLEEP:
			Current_Process->state = SLEEPING;
			Kernel_Dispatch_Next_Task();					
			break;
			
			case CREATE_E:
			Kernel_Create_Event();
			break;
			
			case WAIT_E:
			Kernel_Wait_Event();	
			if(Current_Process->state != RUNNING) 
				Kernel_Dispatch_Next_Task();	//Don't dispatch to a different task if the event is already signaled
			break;
			
			case SIGNAL_E:
			Kernel_Signal_Event();
			Kernel_Dispatch_Next_Task();
			break;
			
			case CREATE_M:
			Kernel_Create_Mutex();
			break;
			
			case LOCK_M:
			Kernel_Lock_Mutex();
			if(Current_Process->state != RUNNING)
				Kernel_Dispatch_Next_Task();		//Task is now waiting for mutex lock
				
			break;
			
			case UNLOCK_M:
			Kernel_Unlock_Mutex();
			if(Current_Process->request == YIELD)		//There are others waiting on the same mutex as well
					Kernel_Dispatch_Next_Task();
			break;
			
			
			case CREATE_SEM:
			Kernel_Create_Semaphore(Current_Process->request_args[0], Current_Process->request_args[1]);
			break;
			
			case GIVE_SEM:
			Kernel_Semaphore_Give(Current_Process->request_args[0], Current_Process->request_args[1]);
			break;
			
			case GET_SEM:
			Kernel_Semaphore_Get(Current_Process->request_args[0], Current_Process->request_args[1]);
			if(Current_Process->state != RUNNING) 
				Kernel_Dispatch_Next_Task();		//Switch task if the process is now waiting for the semaphore
			break;
			
		   
			case YIELD:
			case NONE:					// NONE could be caused by a timer interrupt
			//Current_Process->state = READY;
			Kernel_Dispatch_Next_Task();
			break;
       
			//Invalid request code, just ignore
			default:
				err = INVALID_KERNET_REQUEST_ERR;
			break;
       }
	   
    } 
}

	
/************************************************************************/
/* KERNEL BOOT                                                          */
/************************************************************************/

/*This function initializes the RTOS and must be called before any othersystem calls.*/
void Kernel_Reset()
{
	KernelActive = 0;
	Tick_Count = 0;
	err = NO_ERR;
	
	Scheduler_Reset();
	Task_Reset();
	Event_Reset();
	Mutex_Reset();
	Semaphore_Reset();
	
	#ifdef DEBUG
	printf("OS initialized!\n");
	#endif

}

/* This function starts the RTOS after creating a few tasks.*/
void Kernel_Start()
{
	if (!KernelActive && Task_Count > 0)
	{
		Disable_Interrupt();
		
		/* we may have to initialize the interrupt vector for Enter_Kernel() here. */
			/* here we go...  */
		KernelActive = 1;
		
		/*Initialize and start Timer needed for sleep*/
		Timer_init();
		
		#ifdef DEBUG
		printf("OS begins!\n");
		#endif
		
		Kernel_Handle_Request();
		/* NEVER RETURNS!!! */
	}
}
