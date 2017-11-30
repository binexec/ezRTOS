#include "kernel.h"

#include <stdio.h>
#include <string.h>

#define LED_PIN_MASK 0x80			//Pin 13 = PB7

/*Variables shared within the kernel only*/
volatile static PD Process[MAXTHREAD];			//Contains the process descriptor for all tasks, regardless of their current state.

/*System variables used by the kernel only*/
volatile static unsigned int Last_Dispatched;	//Which task in the process queue was last dispatched.
volatile static unsigned int Tick_Count;		//Number of timer ticks missed

volatile unsigned int Task_Count;				//Number of tasks created so far.


/*Variables accessible by OS*/
volatile PD* Current_Process;					//Process descriptor for the last running process before entering the kernel
volatile unsigned char *KernelSp;				//Pointer to the Kernel's own stack location.
volatile unsigned char *CurrentSp;				//Pointer to the stack location of the current running task. Used for saving into PD during ctxswitch.						//The process descriptor of the currently RUNNING task. CP is used to pass information from OS calls to the kernel telling it what to do.
volatile unsigned int KernelActive;				//Indicates if kernel has been initialzied by OS_Start().
volatile unsigned int Last_PID;					//Last (also highest) PID value created so far.
volatile ERROR_TYPE err;						//Error code for the previous kernel operation (if any)


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

/*
CRITICAL SECTION
*/
void Enter_Critical_Section()
{
	Enable_Interrupt();	
}

void Exit_Critical_Section()
{
	Disable_Interrupt();
}

/************************************************************************/
/*                  ISR FOR HANDLING SLEEP TICKS                        */
/************************************************************************/

//Timer tick ISR
ISR(TIMER1_COMPA_vect)
{
	++Tick_Count;
}

//Processes all tasks that are currently sleeping and decrement their sleep ticks when called. Expired sleep tasks are placed back into their old state
void Kernel_Tick_Handler()
{
	int i;
	
	//No ticks has been issued yet, skipping...
	if(Tick_Count == 0)
		return;
	
	for(i=0; i<MAXTHREAD; i++)
	{
		//Process any active tasks that are sleeping
		if(Process[i].state == SLEEPING)
		{
			//If the current sleeping task's tick count expires, put it back into its READY state
			Process[i].request_arg -= Tick_Count;
			if(Process[i].request_arg <= 0)
			{
				Process[i].state = READY;
				Process[i].request_arg = 0;
			}
		}
		
		//Process any SUSPENDED tasks that were previously sleeping
		else if(Process[i].last_state == SLEEPING)
		{
			//When task_resume is called again, the task will be back into its READY state instead if its sleep ticks expired.
			Process[i].request_arg -= Tick_Count;
			if(Process[i].request_arg <= 0)
			{
				Process[i].last_state = READY;
				Process[i].request_arg = 0;
			}
		}
	}
	Tick_Count = 0;

}

/************************************************************************/
/*                   TASK RELATED KERNEL FUNCTIONS                      */
/************************************************************************/

/* Handles all low level operations for creating a new task */
void Kernel_Create_Task(voidfuncptr f, PRIORITY py, int arg)
{
	int x;
	unsigned char *sp;
	PD *p;

	#ifdef DEBUG
	int counter = 0;
	#endif
	
	//Make sure the system can still have enough resources to create more tasks
	if (Task_Count == MAXTHREAD)
	{
		#ifdef DEBUG
		printf("Task_Create: Failed to create task. The system is at its process threshold.\n");
		#endif
		
		err = MAX_PROCESS_ERR;
		return;
	}

	//Find a dead or empty PD slot to allocate our new task
	for (x = 0; x < MAXTHREAD; x++)
	if (Process[x].state == DEAD) break;
	
	++Task_Count;
	p = &(Process[x]);
	
	/*The code below was agglomerated from Kernel_Create_Task_At;*/
	
	//Initializing the workspace memory for the new task
	sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);
	memset(&(p->workSpace),0,WORKSPACE);

	//Store terminate at the bottom of stack to protect against stack underrun.
	*(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00;

	//Place return address of function at bottom of stack
	*(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00;

	//Allocate the stack with enough memory spaces to save the registers needed for ctxswitch
	#ifdef DEBUG
	 //Fill stack with initial values for development debugging
	 for (counter = 0; counter < 34; counter++)
	 {
		 *(unsigned char *)sp-- = counter;
	 }
	#else
	 //Place stack pointer at top of stack
	 sp = sp - 34;
	#endif
	
	//Build the process descriptor for the new task
	p->pid = ++Last_PID;
	p->pri = py;
	p->arg = arg;
	p->request = NONE;
	p->state = READY;
	p->sp = sp;					/* stack pointer into the "workSpace" */
	p->code = f;				/* function to be executed as a task */
	
	//No errors occured
	err = NO_ERR;
	
}

/*TODO: Check for mutex ownership. If PID owns any mutex, ignore this request*/
static void Kernel_Suspend_Task() 
{
	//Finds the process descriptor for the specified PID
	PD* p = findProcessByPID(Current_Process->request_arg);
	
	//Ensure the PID specified in the PD currently exists in the global process list
	if(p == NULL)
	{
		#ifdef DEBUG
			printf("Kernel_Suspend_Task: PID not found in global process list!\n");
		#endif
		err = PID_NOT_FOUND_ERR;
		return;
	}
	
	//Ensure the task is not in a unsuspendable state
	if(p->state == DEAD || p->state == SUSPENDED)
	{
		#ifdef DEBUG
		printf("Kernel_Suspend_Task: Trying to suspend a task that's in an unsuspendable state %d!\n", p->state);
		#endif
		err = SUSPEND_NONRUNNING_TASK_ERR;
		return;
	}
	
	//Ensure the task is not currently owning a mutex
	/*for(int i=0; i<MAXMUTEX; i++) {
		if (Mutex[i].owner == p->pid) {
			#ifdef DEBUG
			printf("Kernel_Suspend_Task: Trying to suspend a task that currently owns a mutex\n");
			#endif
			err = SUSPEND_NONRUNNING_TASK_ERR;
			return;
		}
	}*/
	
	//Save the process state, and set its current state to SUSPENDED
	if(p->state == RUNNING)
		p->last_state = READY;
	else
		p->last_state = p->state;
		
	p->state = SUSPENDED;
	err = NO_ERR;
}

static void Kernel_Resume_Task()
{
	//Finds the process descriptor for the specified PID
	PD* p = findProcessByPID(Current_Process->request_arg);
	
	//Ensure the PID specified in the PD currently exists in the global process list
	if(p == NULL)
	{
		#ifdef DEBUG
			printf("Kernel_Resume_Task: PID not found in global process list!\n");
		#endif
		err = PID_NOT_FOUND_ERR;
		return;
	}
	
	//Ensure the task is currently in the SUSPENDED state
	if(p->state != SUSPENDED)
	{
		#ifdef DEBUG
		printf("Kernel_Resume_Task: Trying to resume a task that's not SUSPENDED!\n");
		printf("CURRENT STATE: %d\n", p->state);
		#endif
		err = RESUME_NONSUSPENDED_TASK_ERR;
		return;
	}
	
	//Restore the previous state of the task
	if(p->last_state == RUNNING)
		p->state = READY;
	else
		p->state = p->last_state;
		
	p->last_state = SUSPENDED;		//last_state is not needed once a task has resumed, but whatever	
	err = NO_ERR;
	
}



/************************************************************************/
/*                     TASK TERMINATE FUNCTION                         */
/************************************************************************/

static void Kernel_Terminate_Task(void)
{
	MUTEX_TYPE* m;
	// go through all mutex check if it owns a mutex
	/*int index;
	for (index=0; index<MAXMUTEX; index++) 
	{
		if (Mutex[index].owner == Current_Process->pid) {
			// it owns a mutex unlock the mutex
			if (Mutex[index].num_of_process > 0) {
				// if there are other process waiting on the mutex
				PID p_dequeue = 0;
				unsigned int temp_order = Mutex[index].total_num + 1;
				PRIORITY temp_pri = LOWEST_PRIORITY + 1;
				int i;
				for (i=0; i<MAXTHREAD; i++) {
					if (Mutex[index].priority_stack[i] < temp_pri) {
						// found a task with higher priority
						temp_pri = Mutex[index].priority_stack[i];
						temp_order = Mutex[index].order[i];
						p_dequeue = Mutex[index].blocked_stack[i];
						} else if (Mutex[index].priority_stack[i] == temp_pri && temp_order < Mutex[index].order[i]) {
						// same priority and came into the queue earlier
						temp_order = Mutex[index].order[i];
						p_dequeue = Mutex[index].blocked_stack[i];
					}
				}
				//dequeue index i
				Mutex[index].blocked_stack[i] = -1;
				Mutex[index].priority_stack[i] = LOWEST_PRIORITY+1;
				Mutex[index].order[i] = 0;
				--(Mutex[index].num_of_process);
				PD* target_p = findProcessByPID(p_dequeue);
				Mutex[index].owner = p_dequeue;
				Mutex[index].own_pri = temp_pri;			//keep track of new owner's priority;
				target_p->state = READY;
			} else {
				Mutex[index].owner = 0;
				Mutex[index].count = 0;
			}
		}
	}*/
	
	Current_Process->state = DEAD;			//Mark the task as DEAD so its resources will be recycled later when new tasks are created
	--Task_Count;
	
	PORTB &= ~(1<<PB2);
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
		if(Process[j].state == READY && Process[j].pri < highest_priority)
		{
			next_dispatch = j;
			highest_priority = Process[next_dispatch].pri;
		}
	}
	
	return next_dispatch;
}

/* Dispatches a new task */
static void Dispatch()
{
	unsigned int i;
	int next_dispatch = Kernel_Select_Next_Task();

	//When none of the tasks in the process list is ready
	if(next_dispatch < 0)
	{
		i = Last_Dispatched;
		
		//We'll temporarily re-enable interrupt in case if one or more task is waiting on events/interrupts or sleeping
		Enable_Interrupt();
		
		//Looping through the process list until any process becomes ready
		while(Process[i].state != READY)
		{
			//Increment process index
			i = (i+1) % MAXTHREAD;
			
			//Check if any timer ticks came in
			Disable_Interrupt();
			Kernel_Tick_Handler();	
			Enable_Interrupt();
		}
		
		//Now that we have some ready tasks, interrupts must be disabled for the kernel to function properly again.
		Disable_Interrupt();
		next_dispatch = Kernel_Select_Next_Task();
		
		if(next_dispatch < 0)
			printf("SOMETHING WENT WRONG WITH SCHEDULLER!\n");
		
	}
	

	//Load the next selected task's process descriptor into Cp
	Last_Dispatched = next_dispatch;
	Current_Process = &(Process[Last_Dispatched]);
	CurrentSp = Current_Process->sp;
	Current_Process->state = RUNNING;
}

/**
  * This internal kernel function is the "main" driving loop of this full-served
  * model architecture. Basically, on OS_Start(), the kernel repeatedly
  * requests the next user task's next system call and then invokes the
  * corresponding kernel function on its behalf.
  *
  * This is the main loop of our kernel, called by OS_Start().
  */
static void Next_Kernel_Request() 
{
	Dispatch();	//Select an initial task to run

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

		/* if this task makes a system call, it will return to here! */

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
			Dispatch();					//Dispatch is only needed if the syscall requires running a different task  after it's done
			break;
		   
			case SUSPEND:
			Kernel_Suspend_Task();
			break;
			
			case RESUME:
			Kernel_Resume_Task();
			break;
			
			case SLEEP:
			Current_Process->state = SLEEPING;
			Dispatch();					
			break;
			
			case CREATE_E:
			Kernel_Create_Event();
			break;
			
			case WAIT_E:
			Kernel_Wait_Event();	
			if(Current_Process->state != RUNNING) Dispatch();	//Don't dispatch to a different task if the event is already signaled
			break;
			
			case SIGNAL_E:
			Kernel_Signal_Event();
			Dispatch();
			break;
			
			case CREATE_M:
			Kernel_Create_Mutex();
			break;
			
			case LOCK_M:
			Kernel_Lock_Mutex();
			//Maybe add a dispatch() here if lock fails?
			break;
			
			case UNLOCK_M:
			Kernel_Unlock_Mutex();
			//Does this need dispatch under any circumstances?
			break;
		   
			case YIELD:
			case NONE:					// NONE could be caused by a timer interrupt
			Current_Process->state = READY;
			Dispatch();
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

/*Sets up the timer needed for task_sleep*/
void Timer_init()
{
	/*Timer1 is configured for the task*/
	
	//Use Prescaler = 256
	TCCR1B |= (1<<CS12);
	TCCR1B &= ~((1<<CS11)|(1<<CS10));
	
	//Use CTC mode (mode 4)
	TCCR1B |= (1<<WGM12);
	TCCR1B &= ~((1<<WGM13)|(1<<WGM11)|(1<<WGM10));
	
	OCR1A = TICK_LENG;			//Set timer top comparison value to ~10ms
	TCNT1 = 0;					//Load initial value for timer
	TIMSK1 |= (1<<OCIE1A);      //enable match for OCR1A interrupt
	
	#ifdef DEBUG
	printf("Timer initialized!\n");
	#endif
}

/*This function initializes the RTOS and must be called before any othersystem calls.*/
void Kernel_Reset()
{
	int x;
	
	Task_Count = 0;
	KernelActive = 0;
	Tick_Count = 0;
	Last_Dispatched = 0;
	Last_PID = 0;
	
	err = NO_ERR;
	
	//Clear and initialize the memory used for tasks
	memset(Process, 0, MAXTHREAD*sizeof(PD));
	for (x = 0; x < MAXTHREAD; x++) {
		Process[x].state = DEAD;
	}
	
	
	Event_Reset();
	Mutex_Reset();
	
	#ifdef DEBUG
	printf("OS initialized!\n");
	#endif
	
	DDRB = (1<<PB2);	//pin 51
}

/* This function starts the RTOS after creating a few tasks.*/
void Kernel_Start()
{
	if ( (!KernelActive) && (Task_Count > 0))
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
		
		Next_Kernel_Request();
		/* NEVER RETURNS!!! */
	}
}
