#include "task.h"


volatile PD Process[MAXTHREAD];			//Contains the process descriptor for all tasks, regardless of their current state.
volatile unsigned int Task_Count;				//Number of tasks created so far.
volatile unsigned int Last_PID;					//Last (also highest) PID value created so far.


void Task_Reset()
{
	Task_Count = 0;

	//Clear and initialize the memory used for tasks
	memset(Process, 0, MAXTHREAD*sizeof(PD));
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
		if(Process[x].state == DEAD) break;
	
	++Task_Count;
	p = &(Process[x]);
	
	/*The code below was agglomerated from Kernel_Create_Task_At;*/\
	
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
	
	//No errors occured
	err = NO_ERR;
	
}

/*TODO: Check for mutex ownership. If PID owns any mutex, ignore this request*/
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



void Kernel_Terminate_Task(void)
{
	Current_Process->state = DEAD;			//Mark the task as DEAD so its resources will be recycled later when new tasks are created
	--Task_Count;
}