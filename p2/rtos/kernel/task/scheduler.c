#include "scheduler.h"


volatile static unsigned int Last_Dispatched;			//Which task in the process queue was last dispatched.

//For Preemptive Cswitch
#ifdef PREEMPTIVE_CSWITCH_FREQ
volatile static unsigned int Preemptive_Cswitch_Allowed;
volatile static unsigned int Ticks_Since_Last_Cswitch;
#endif



/************************************************************************/
/*                  HANDLING TIMER TICK INTERRUPTS						*/
/************************************************************************/

void Kernel_Dispatch_Next_Task();

//This function is called ONLY by the timer ISR. Therefore, it will not enter the kernel through regular mode
void Scheduler_Tick_ISR()
{
	if(!Preemptive_Cswitch_Allowed)
		return;

	//Preemptive Scheduling: Has it been a long time since we switched to a new task?
	if(++Ticks_Since_Last_Cswitch >= PREEMPTIVE_CSWITCH_FREQ)
	{
		Disable_Interrupt();
		Kernel_Dispatch_Next_Task();
		CSwitch();						//same as Exit_Kernel(), interrupts are automatically enabled at the end
		Enable_Interrupt();
	}
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
void Kernel_Dispatch_Next_Task()
{
	unsigned int i, j;
	int next_dispatch;
	
	//Don't allow preemptive cswitch to kick in again while we're waiting for a task to be ready
	#ifdef PREEMPTIVE_CSWITCH
	Preemptive_Cswitch_Allowed = 0;
	#endif
	
	Current_Process->state = READY;		//Mark the current task from RUNNING to READY, so it can be considered by the scheduler again
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
/*								 BOOTING								*/
/************************************************************************/

void Scheduler_Reset()
{
	Last_Dispatched = 0;
	
	#ifdef PREEMPTIVE_CSWITCH
	Preemptive_Cswitch_Allowed = 1;
	Ticks_Since_Last_Cswitch = 0;
	#endif
}
