#include "kernel.h"


/*System variables used by the kernel only*/		
volatile static unsigned int Tick_Count;				//Number of timer ticks missed
volatile static PtrList* Last_Dispatched;				//Pointer to the global process queue of the task that was running previously

//For Preemptive Cswitch
#ifdef PREEMPTIVE_CSWITCH
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


//uchar Kernel_Heap[KERNEL_HEAP_SIZE];							//Heap for kerel object allocation, used by kmalloc
//uchar Task_Workspace_Pool[WORKSPACE_HEAP_SIZE];				//Heap for kerel object allocation, used by kmalloc
//volatile void* Workspace_Pool_End;





/************************************************************************/
/*						  KERNEL-ONLY HELPERS                           */
/************************************************************************/

/*Returns the pointer of a process descriptor in the global process list, by searching for its PID*/
PD* findProcessByPID(int pid)
{
	PtrList *i;
	PD *process_i;
	
	//Valid PIDs must be greater than 0.
	if(pid <= 0)
		return NULL;
	
	for(i = &ProcessList; i; i = i->next)
	{
		process_i = (PD*)i->ptr;
		if (process_i->pid == pid)
			return process_i;
	}
	
	//No process with such PID
	return NULL;
}

/*Returns the PID associated with a function's memory address. Used by the OS*/
int findPIDByFuncPtr(taskfuncptr f)
{
	PtrList *i;
	PD *process_i;
	
	for(i = &ProcessList; i; i = i->next)
	{
		process_i = (PD*)i->ptr;
		if (process_i->code == f)
			return process_i->pid;
	}
	
	//No process with such PID
	return -1;
}


void print_processes()
{
	PtrList *i;
	PD *process_i;
	
	for(i = &ProcessList; i; i = i->next)
	{
		process_i = (PD*)i->ptr;
		if(process_i->state != DEAD)
		printf("\tPID: %d\t State: %d\t Priority: %d\t Timeout: %d\n", process_i->pid, process_i->state, process_i->pri, process_i->request_timeout);
	}
	printf("\n");
}

void print_process(PID p)
{
	PD *pd = findProcessByPID(p);

	printf("\tPID: %d\t State: %d\t Priority: %d\t Timeout: %d\n", pd->pid, pd->state, pd->pri, pd->request_timeout);
	printf("\n");
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
	
	//Preemptive Scheduling: Has it been a long time since we switched to a new task?
	#ifdef PREEMPTIVE_CSWITCH	
	if(!Preemptive_Cswitch_Allowed)
		return;
		
	if(++Ticks_Since_Last_Cswitch >= PREEMPTIVE_CSWITCH_FREQ)
	{
		Disable_Interrupt();
		Current_Process->request = YIELD;
		Enter_Kernel();							//Interrupts are automatically enabled once kernel is exited
	}
	#endif
}

//Processes all tasks that are currently sleeping and decrement their sleep ticks when called. Expired sleep tasks are placed back into their old state
static void Kernel_Tick_Handler()
{
	volatile PtrList *i;
	volatile PD *process_i;
	int remaining_ticks;
	
	//No new ticks has been issued yet, skipping...
	if(Tick_Count == 0)
		return;
	
	for(i = &ProcessList; i; i = i->next)
	{
		process_i = (PD*)i->ptr;
		
		//Increment tick count for starvation prevention
		#ifdef PREVENT_STARVATION
		if(process_i->state == READY)
			process_i->starvation_ticks += Tick_Count;
		#endif
		
		//Skip tasks that do not have any outstanding ticks remaining (or at all in the first place)
		if(process_i->request_timeout == 0)
			continue;
		
		//Decrement the request timeout counter for any tasks having a nonzero counter; wake up any tasks if their counter has reached zero
		remaining_ticks = process_i->request_timeout - Tick_Count;
		if(remaining_ticks > 0)
		{
			process_i->request_timeout = remaining_ticks;
			continue;
		}

		process_i->request_timeout = 0;
		
		if(process_i->state == SUSPENDED)		//"Thaw" any SUSPENDED tasks but do not wake them up immediately
			process_i->last_state = READY;
		else									//Wake up any other tasks timing out from its request (including sleep), and set its return value to 0 indicate a failure.
		{
			process_i->state = READY;
			process_i->request_ret = 0;
		}
	}
	
	Tick_Count = 0;
}



/************************************************************************/
/*                     KERNEL SCHEDULING FUNCTIONS                      */
/************************************************************************/

//Look through all ready tasks and select the next task for dispatching
static PtrList* Kernel_Select_Next_Task()
{
	PtrList *i;
	PD *process_i;
	int j;
	
	PtrList *next_dispatch = NULL;
	int highest_priority = LOWEST_PRIORITY + 1;	
	
	#ifdef PREVENT_STARVATION
	PtrList *most_starved = NULL;
	PD *process_ms;
	#endif	
	
	//Iterate through every task in the process list, starting from the task AFTER the one that was previously dispatched
	for(j=0, i = ptrlist_cnext(&ProcessList,Last_Dispatched); j<Task_Count; j++, i = ptrlist_cnext(&ProcessList,i))
	{
		process_i = (PD*)i->ptr;

		if(process_i->state != READY)
			continue;
		
		//Select the next READY task with the highest priority. If all READY tasks have the same priority, the next task of the same priority is dispatched (round robin)
		if(process_i->pri < highest_priority)
		{
			next_dispatch = i;
			highest_priority = process_i->pri;
		}
			
		//If a READY task is found to be starving, record it if it has the highest starving count
		#ifdef PREVENT_STARVATION
		else if(process_i->starvation_ticks >= STARVATION_MAX)
		{
			if(most_starved && process_ms->starvation_ticks <= process_i->starvation_ticks)
				continue;
			
			most_starved = i;
			process_ms = process_i;
		}
		#endif
	}
	
	#ifdef PREVENT_STARVATION
	if(most_starved)
		return most_starved;
	#endif
	
	return next_dispatch;
}

/* Dispatches a new task */
static void Kernel_Dispatch_Next_Task()
{
 
	volatile PtrList* i;
	volatile PD *process_i;
	unsigned int j;
	
	PtrList* next_dispatch;
	
	//Don't allow preemptive cswitch to kick in again while we're waiting for a task to be ready
	#ifdef PREEMPTIVE_CSWITCH
	Preemptive_Cswitch_Allowed = 0;
	#endif
	
	//Mark the current task from RUNNING to READY, so it can be considered by the scheduler again
	if(Current_Process->state == RUNNING)
		Current_Process->state = READY;			
	
	//Check if any timer ticks came in, so more tasks can be ready for dispatching
	Kernel_Tick_Handler();
	
	//Select a READY task for dispatch
	next_dispatch = Kernel_Select_Next_Task();

	//When none of the tasks in the process list is ready
	if(!next_dispatch)
	{
		i = Last_Dispatched;
		process_i = (PD*)i->ptr;
		j = 0;

		//We'll temporarily re-enable interrupt in case if one or more task is waiting on events/interrupts or sleeping
		Enable_Interrupt();
		
		//Looping through the process list until any process becomes ready
		while(process_i->state != READY)
		{			
			//We only need to check if any timer ticks came in periodically
			if(++j >= Task_Count*100)
			{
				Disable_Interrupt();
				Kernel_Tick_Handler();
				j = 0;
				Enable_Interrupt();
			}

			i = ptrlist_cnext(&ProcessList,i);
			process_i = (PD*)i->ptr;
		}
		
		//Now that we have some ready tasks, interrupts must be disabled for the kernel to function properly again.
		Disable_Interrupt();
		next_dispatch = Kernel_Select_Next_Task();	
	}

	//Load the next selected task's process descriptor into Cp
	Last_Dispatched = next_dispatch;
	Current_Process = (PD*)next_dispatch->ptr;
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

static void Kernel_Main_Loop() 
{
	//Select an initial task to run
	Last_Dispatched = ptrlist_findtail(&ProcessList);
	Kernel_Dispatch_Next_Task();

	//After OS initialization, THIS WILL BE KERNEL'S MAIN LOOP!
	//NOTE: When another task makes a syscall and enters the loop, it's still in the RUNNING state!
	while(1) 
	{

		/********************************************************************************************/
		/*																							*/
		/*	When a task makes a kernel call, it will end up HERE after Enter_Kernel() is invoked.	*/	
		/*	The calling task's process descriptor is loaded as Current_Process.						*/
		/*																							*/
		/********************************************************************************************/

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
			
			case DESTROY_M:
			Kernel_Destroy_Mutex();
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
			
			case DESTROY_SEM:
			Kernel_Destroy_Semaphore();
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
			
			case DESTROY_EG:
			Kernel_Destroy_Event_Group();
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
		
		//Clears the process' request field after it has been handled
		Current_Process->request = NONE;
		
		//Switch to a new task if the completed kernel request requires it
		if(Kernel_Request_Cswitch)
		{
			Kernel_Dispatch_Next_Task();
			Kernel_Request_Cswitch = 0;
		}
		
		//Load the newly selected task's stack pointer and switch to its context
		CurrentSp = Current_Process->sp;
		Exit_Kernel();
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
	Last_Dispatched = NULL;
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
	//printf("Semaphores available\n");
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
		
		Kernel_Main_Loop();
		/* NEVER RETURNS!!! */
	}
}
