#include "kernel.h"

#include <stdio.h>
#include <string.h>

#define LED_PIN_MASK 0x80			//Pin 13 = PB7


/*System variables used by the kernel only*/
volatile static unsigned int Last_Dispatched;			//Which task in the process queue was last dispatched.
volatile static unsigned int Tick_Count;				//Number of timer ticks missed


//For Preemptive Cswitch
#ifdef PREEMPTIVE_CSWITCH_FREQ
volatile static unsigned int Preemptive_Cswitch_Allowed;
volatile static unsigned int Ticks_Since_Last_Cswitch;
#endif


/*Variables accessible by OS and other kernel modules*/
volatile PD* Current_Process;						//Process descriptor for the last running process before entering the kernel
volatile unsigned char *KernelSp;					//Pointer to the Kernel's own stack location.
volatile unsigned char *CurrentSp;					//Pointer to the stack location of the current running task. Used for saving into PD during ctxswitch.						//The process descriptor of the currently RUNNING task. CP is used to pass information from OS calls to the kernel telling it what to do.
volatile unsigned int KernelActive;					//Indicates if kernel has been initialzied by OS_Start().
volatile unsigned int Kernel_Request_Cswitch;		//If a kernel request set this variable to 1, the kernel will switch to a different task after the request completes
volatile ERROR_CODE err;							//Error code for the previous kernel operation (if any)		


//uchar Kernel_Heap[KERNEL_HEAP_SIZE];				//Heap for kerel object allocation, used by kmalloc


/************************************************************************/
/*						  KERNEL-ONLY HELPERS                           */
/************************************************************************/

/*Returns the pointer of a process descriptor in the global process list, by searching for its PID*/
PD* findProcessByPID(int pid)
{
	PtrList *i;
	PD *pd_i;
	
	//Valid PIDs must be greater than 0.
	if(pid <= 0)
		return NULL;
	
	for(i = &Process; i; i = i->next)
	{
		pd_i = (PD*)i->ptr;
			
		if (pd_i->pid == pid)
			return pd_i;
	}
	
	//No process with such PID
	return NULL;
}


/*void print_processes()
{
	int i;
	
	for(i=0; i<MAXTHREAD; i++)
	{
		if(Process[i].state != DEAD)
			printf("\tPID: %d\t State: %d\t Priority: %d\n", Process[i].pid, Process[i].state, Process[i].pri);
	}
	printf("\n");
}*/

/*Returns the PID associated with a function's memory address. Used by the OS*/
int findPIDByFuncPtr(taskfuncptr f)
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
/*                  KERNEL TIMER TICK HANDLERS							*/
/************************************************************************/

static void Kernel_Dispatch_Next_Task();
static void Kernel_Tick_Handler();

//This function is called ONLY by the timer ISR. Therefore, it will not enter the kernel through regular modes
void Kernel_Tick_ISR()
{
	//Increment the system-wide missed tick count
	++Tick_Count;
	
	#ifdef PREEMPTIVE_CSWITCH	
	if(!Preemptive_Cswitch_Allowed)
		return;

	//Preemptive Scheduling: Has it been a long time since we switched to a new task?
	if(++Ticks_Since_Last_Cswitch >= PREEMPTIVE_CSWITCH_FREQ)
	{
		Disable_Interrupt();
		Kernel_Dispatch_Next_Task();
		CSwitch();						//same as Exit_Kernel(), interrupts are automatically enabled at the end
		//Enable_Interrupt();
	}
	#endif
}

//Processes all tasks that are currently sleeping and decrement their sleep ticks when called. Expired sleep tasks are placed back into their old state
static void Kernel_Tick_Handler()
{
	int i;
	int remaining_ticks;
	
	//No new ticks has been issued yet, skipping...
	if(Tick_Count == 0)
		return;
	
	for(i=0; i<MAXTHREAD; i++)
	{
		//Increment tick count for starvation prevention
		#ifdef PREVENT_STARVATION
		if(Process[i].state == READY)
			Process[i].starvation_ticks += Tick_Count;
		#endif
		
		//Skip tasks that do not have any outstanding ticks remaining (or at all in the first place)
		if(Process[i].request_timeout == 0)
			continue;
		
		remaining_ticks = Process[i].request_timeout - Tick_Count;
		if(remaining_ticks > 0)
		{
			Process[i].request_timeout = remaining_ticks;
			continue;
		}
		
		//Wake up the task once its timer has expired
		Process[i].request_timeout = 0;
		if(Process[i].state == SLEEPING)		//Wake up sleeping tasks
			Process[i].state = READY;
				
		else if(Process[i].state == SUSPENDED && Process[i].last_state == SLEEPING)		//Wake up SUSPENDED sleeping tasks (should we do this for other suspended states too?)
			Process[i].last_state = READY;
			
		else	//Wake up any other tasks timing out from its request, and set its return value to 0 indicate a failure.
		{
			Process[i].state = READY;
			Process[i].request_ret = 0;
		}
	}
	Tick_Count = 0;
}



/************************************************************************/
/*                     KERNEL SCHEDULING FUNCTIONS                      */
/************************************************************************/

//Look through all ready tasks and select the next task for dispatching
static int Kernel_Select_Next_Task()
{
	unsigned int i = 0, j = Last_Dispatched;
	int highest_priority = LOWEST_PRIORITY + 1;
	int next_dispatch = -1;
	
	//Find the next READY task with the highest priority by iterating through the process list ONCE.
	//If all READY tasks have the same priority, the next task of the same priority is dispatched (round robin)
	for(i=0; i<MAXTHREAD; i++)
	{
		//Increment process index
		j = (j+1) % MAXTHREAD;
		
		//Select the READY process with the highest priority
		if(Process[j].state == READY)
		{
			if(Process[j].pri < highest_priority)
			{
				next_dispatch = j;
				highest_priority = Process[next_dispatch].pri;
			}
			
			//If a READY task is found to be starving, select it immediately regardless of its priority
			//TODO: Find the task that has been starved the most, rather than the first?
			#ifdef PREVENT_STARVATION
			else if(Process[j].starvation_ticks >= STARVATION_MAX)
			{
				printf("Found Starving task %d, tick count %d!\n", Process[j].pid, Process[j].starvation_ticks);
				next_dispatch = j;
				break;
			}
			#endif
		}
	}
	
	return next_dispatch;
}

/* Dispatches a new task */
static void Kernel_Dispatch_Next_Task()
{
	unsigned int i, j;
	int next_dispatch;
	
	//Don't allow preemptive cswitch to kick in again while we're waiting for a task to be ready
	#ifdef PREEMPTIVE_CSWITCH
	Preemptive_Cswitch_Allowed = 0;
	#endif
	
	//Mark the current task from RUNNING to READY, so it can be considered by the scheduler again
	if(Current_Process->state == RUNNING)
		Current_Process->state = READY;			
	
	//Check if any timer ticks came in
	Kernel_Tick_Handler();
	
	next_dispatch = Kernel_Select_Next_Task();

	//When none of the tasks in the process list is ready
	if(next_dispatch < 0)
	{
		i = Last_Dispatched;
		j = 0;

		//We'll temporarily re-enable interrupt in case if one or more task is waiting on events/interrupts or sleeping
		Enable_Interrupt();
		
		//Looping through the process list until any process becomes ready
		while(Process[i].state != READY)
		{			
			//Increment process index
			i = (i+1) % MAXTHREAD;
			
			//Check if any timer ticks came in periodically
			if(j++ >= MAXTHREAD*10)
			{
				Disable_Interrupt();
				Kernel_Tick_Handler();
				j = 0;
				Enable_Interrupt();
			}
		}
		
		//Now that we have some ready tasks, interrupts must be disabled for the kernel to function properly again.
		Disable_Interrupt();
		next_dispatch = Kernel_Select_Next_Task();		
	}
	
	//Stash away the current running task into the ready queue
	/*if(Current_Process->state == RUNNING)
		Current_Process->state = READY;*/

	//Load the next selected task's process descriptor into Cp
	Last_Dispatched = next_dispatch;
	Current_Process = &(Process[Last_Dispatched]);
	CurrentSp = Current_Process->sp;
	Current_Process->state = RUNNING;
	
	//printf("Dispatching PID:%d\n", Current_Process->pid);
		
	//Reset Preemptive cswitch
	#ifdef PREEMPTIVE_CSWITCH
	Ticks_Since_Last_Cswitch = 0;
	Preemptive_Cswitch_Allowed = 1;
	#endif
	
	//Reset Starvation Count
	#ifdef PREVENT_STARVATION
	Current_Process->starvation_ticks = 0;
	#endif
}



/************************************************************************/
/*						KERNEL REQUEST HANDLER                          */
/************************************************************************/


//What happens when an invalid request was made?
static void Kernel_Invalid_Request()
{
	#ifdef DEBUG
	printf("***INVALID KERNEL REQUEST!***\n");
	#endif
	
	err = INVALID_KERNET_REQUEST_ERR;
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

		//Load the current task's stack pointer and switch to its context
		CurrentSp = Current_Process->sp;
		Exit_Kernel();


		/************************************************************************/
		/*     if this task makes a system call, it will return to here!		*/
		/************************************************************************/
		
		
		//Save the current task's stack pointer and proceed to handle its request
		Current_Process->sp = CurrentSp;

		//Because each branch only calls a function, this switch statement should hopefully be converted to a jump table by the compiler
		switch(Current_Process->request)
		{
			case CREATE_T:
			Kernel_Create_Task();
			break;
			
			case TERMINATE:
			Kernel_Terminate_Task();
			break;
		   
			case SUSPEND:
			Kernel_Suspend_Task();
			break;
			
			case RESUME:
			Kernel_Resume_Task();
			break;
			
			case SLEEP:
			Kernel_Sleep_Task();					
			break;
			
			
			#ifdef EVENT_ENABLED
			case CREATE_E:
			Kernel_Create_Event();
			break;
			
			case WAIT_E:
			Kernel_Wait_Event();	
			break;
			
			case SIGNAL_E:
			Kernel_Signal_Event();
			break;
			#endif
			
			
			#ifdef MUTEX_ENABLED
			case CREATE_M:
			Kernel_Create_Mutex();
			break;
			
			case LOCK_M:
			Kernel_Lock_Mutex();
			break;
			
			case UNLOCK_M:
			Kernel_Unlock_Mutex();
			break;
			#endif
			
			
			#ifdef SEMAPHORE_ENABLED
			case CREATE_SEM:
			Kernel_Create_Semaphore();
			break;
			
			case GIVE_SEM:
			Kernel_Semaphore_Give();
			break;
			
			case GET_SEM:
			Kernel_Semaphore_Get();
			break;
			#endif
			
			#ifdef EVENT_GROUP_ENABLED
			case CREATE_EG:
			Kernel_Create_Event_Group();
			break;
			
			case SET_EG_BITS:
			Kernel_Event_Group_Set_Bits();
			break;
			
			case CLEAR_EG_BITS:
			Kernel_Event_Group_Clear_Bits();
			break;
			
			case WAIT_EG:
			Kernel_Event_Group_Wait_Bits();
			break;
			
			case GET_EG_BITS:
			Kernel_Event_Group_Get_Bits();
			break;
			#endif
			
		   
			case YIELD:
			case NONE:							// NONE could be caused by a timer interrupt
			Kernel_Dispatch_Next_Task();
			break;
       
			default:							//Invalid request code, just ignore
			Kernel_Invalid_Request();
			break;
       }
	   
		//Switch to a new task if the completed kernel request requires it
		if(Kernel_Request_Cswitch)
		{
			Kernel_Dispatch_Next_Task();
			Kernel_Request_Cswitch = 0;
		}
    } 
}

	
/************************************************************************/
/*							KERNEL BOOT									*/
/************************************************************************/

/*This function initializes the RTOS and must be called before any othersystem calls.*/
void Kernel_Reset()
{
	KernelActive = 0;
	Tick_Count = 0;
	Last_Dispatched = 0;
	Kernel_Request_Cswitch = 0;
	
	#ifdef PREEMPTIVE_CSWITCH
	Preemptive_Cswitch_Allowed = 1;
	Ticks_Since_Last_Cswitch = 0;
	#endif
	
	err = NO_ERR;
	
	Task_Reset();
	
	#ifdef EVENT_ENABLED
	Event_Reset();
	#endif
	
	#ifdef EVENT_GROUP_ENABLED
	Event_Group_Reset();
	#endif
	
	#ifdef MUTEX_ENABLED
	Mutex_Reset();
	#endif
	
	#ifdef SEMAPHORE_ENABLED
	Semaphore_Reset();
	#endif
	
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
