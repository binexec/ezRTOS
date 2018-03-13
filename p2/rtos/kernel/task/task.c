#include "task.h"
#include <stdlib.h>		//Remove once kmalloc is used


volatile PtrList ProcessList;					//Contains the process descriptor for all tasks, regardless of their current state.
volatile unsigned int Task_Count;				//Number of tasks created so far.
volatile unsigned int Last_PID;					//Last (also highest) PID value created so far.


void Task_Reset()
{
	Task_Count = 0;
	Last_PID = 0;

	ProcessList.ptr = NULL;
	ProcessList.next = NULL;
}


/************************************************************************/
/*                   TASK RELATED KERNEL FUNCTIONS                      */
/************************************************************************/

PID Kernel_Create_Task_Direct(taskfuncptr f, PRIORITY py, int arg)
{
	unsigned char *sp;
	PD *p;
	
	//Make sure the system can still have enough resources to create more tasks
	if (Task_Count == MAXTHREAD)
	{
		#ifdef DEBUG
		printf("Kernel_Create_Task: Failed to create task. There are no free slots remaining\n");
		#endif
		
		err = MAX_PROCESS_ERR;
		if(KernelActive)
			Current_Process->request_ret = 0;
		return 0;
	}
	
	p = malloc(sizeof(PD));
	ptrlist_add(&ProcessList, p);
	++Task_Count;
	
	/*The code below was agglomerated from Kernel_Create_Task_At;*/
	
	//Initializing the workspace memory for the new task
	sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);
	memset(&(p->workSpace), 0, WORKSPACE);
	Kernel_Init_Task_Stack(&sp, f);

	//Build the process descriptor for the new task
	p->pid = ++Last_PID;
	p->pri = py;
	p->arg = arg;
	p->request = NONE;
	p->state = READY;
	p->sp = sp;					/* stack pointer into the "workSpace" */
	p->code = f;				/* function to be executed as a task */
	
	#ifdef PREVENT_STARVATION
	p->starvation_ticks = 0;
	#endif
	
	err = NO_ERR;
	if(KernelActive)
		Current_Process->request_ret = p->pid;
	return p->pid;
}


//For creating a new task dynamically when the kernel is already running
PID Kernel_Create_Task()
{
	#define req_func_pointer	(taskfuncptr)Current_Process->request_args[0]
	#define req_priority		Current_Process->request_args[1]
	#define req_taskarg			Current_Process->request_args[2]
	
	return Kernel_Create_Task_Direct(req_func_pointer, req_priority, req_taskarg);
	
	#undef req_func_pointer
	#undef req_priority
	#undef req_taskarg
}


void Kernel_Suspend_Task() 
{
	//Finds the process descriptor for the specified PID
	PD* p = findProcessByPID(Current_Process->request_args[0]);
	
	//Ensure the PID specified in the PD currently exists in the global process list
	if(p == NULL)
	{
		#ifdef DEBUG
			printf("Kernel_Suspend_Task: PID not found in global process list!\n");
		#endif
		err = PID_NOT_FOUND_ERR;
		return;
	}
	
	//Do not suspend tasks that are dead or waiting on resources. This may cause deadlocks. 
	//Maybe optionally implement "deferred suspension" for tasks waiting on resources?
	if(!(p->state == READY || p->state == RUNNING || p->state == SLEEPING))
	{
		#ifdef DEBUG
		printf("Kernel_Suspend_Task: Trying to suspend a task that's in an unsuspendable state %d!\n", p->state);
		#endif
		err = UNSUSPENDABLE_TASK_STATE_ERR;
		return;
	}
	
	//Save the process state, and set its current state to SUSPENDED
	if(p->state == RUNNING)
		p->last_state = READY;
	else
		p->last_state = p->state;
		
	p->state = SUSPENDED;
	err = NO_ERR;
}


void Kernel_Resume_Task()
{
	//Finds the process descriptor for the specified PID
	PD* p = findProcessByPID(Current_Process->request_args[0]);
	
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


void Kernel_Sleep_Task(void)
{
	//Sleep ticks is already set by the OS call
	Current_Process->state = SLEEPING;
	Kernel_Request_Cswitch = 1;
}


void Kernel_Terminate_Task(void)
{
	Current_Process->state = DEAD;	
	ptrlist_remove(&ProcessList, ptrlist_find(&ProcessList, Current_Process));		//Free the PD used by the terminated task
	--Task_Count;
	
	Kernel_Request_Cswitch = 1;
}