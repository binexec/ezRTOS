#include "mutex.h"

#include <string.h>

volatile static MUTEX_TYPE Mutex[MAXMUTEX];		//Contains all the mutex objects
volatile unsigned int Mutex_Count;				//Number of Mutexes created so far.
volatile unsigned int Last_MutexID;				//Last (also highest) MUTEX value created so far.

/************************************************************************/
/*						USED DURING BOOTING                             */
/************************************************************************/

void Mutex_Reset()
{
	Mutex_Count = 0;
	Last_MutexID = 0;
	
	//Clear and initialize the memory used for Mutex
	memset(Mutex, 0, MAXMUTEX*sizeof(MUTEX_TYPE));
}

/************************************************************************/
/*						  KERNEL-ONLY HELPERS                           */
/************************************************************************/

MUTEX_TYPE* findMutexByMutexID(MUTEX m)
{
	int i;
	
	//Ensure the request mutex ID is > 0
	if(m <= 0)
	{
		#ifdef DEBUG
		printf("findMutexByID: The specified mutex ID is invalid!\n");
		#endif
		err = INVALID_ARG_ERR;
		return NULL;
	}
	
	//Find the requested Mutex and return its pointer if found
	for(i=0; i<MAXMUTEX; i++)
	{
		if(Mutex[i].id == m)
		return &Mutex[i];
	}
	
	//mutex wasn't found
	//#ifdef DEBUG
	//printf("findMutexByEventID: The requested mutex %d was not found!\n", m);
	//#endif
	err = MUTEX_NOT_FOUND_ERR;
	return NULL;
}

/************************************************************************/
/*                  MUTEX RELATED KERNEL FUNCTIONS                      */
/************************************************************************/

void Kernel_Create_Mutex(void)
{
	int i;
	
	//Make sure the system's mutexes are not at max
	if(Mutex_Count >= MAXMUTEX)
	{
		#ifdef DEBUG
		printf("Kernel_Create_Mutex: Failed to create Mutex. The system is at its max mutex threshold.\n");
		#endif
		err = MAX_MUTEX_ERR;
		return;
	}
	
	//Find an uninitialized Mutex slot
	for(i=0; i<MAXMUTEX; i++)
	if(Mutex[i].id == 0) break;
	
	//Assign a new unique ID to the mutex. Note that the smallest valid mutex ID is 1.
	Mutex[i].id = ++Last_MutexID;
	Mutex[i].owner = 0;		// note when mutex's owner is 0, it is free
	
	// init priority stack
	for (int j=0; j<MAXTHREAD; j++) 
	{
		Mutex[i].priority_stack[j] = LOWEST_PRIORITY+1;
		Mutex[i].blocked_stack[j] = -1;
		Mutex[i].order[j] = 0;
	}
	Mutex[i].num_of_process = 0;
	Mutex[i].total_num = 0;
	++Mutex_Count;
	err = NO_ERR;
	
	#ifdef DEBUG
	printf("Kernel_Create_Mutex: Created Mutex %d!\n", Last_MutexID);
	#endif

}



void Kernel_Lock_Mutex(void)
{
	MUTEX_TYPE* m = findMutexByMutexID(Current_Process->request_arg);
	PD *m_owner = findProcessByPID(m->owner);
	
	if(m == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Lock_Mutex: Error finding requested mutex!\n");
		#endif
		return;
	}
	
	// if mutex is free
	if(m->owner == 0)
	{
		m->owner = Current_Process->pid;
		m->count = 1;
		m->own_pri = Current_Process->pri;				// keep track of the original priority of the owner
		
		return;
		} else if (m->owner == Current_Process->pid) {
		// if it has locked by the current process
		++(m->count);
		return;
		} else {
		Current_Process->state = WAIT_MUTEX;								//put cp into state wait mutex
		//enqueue cp to stack
		++(m->num_of_process);
		++(m->total_num);
		for (int i=0; i<MAXTHREAD; i++) 
		{
			if (m->blocked_stack[i] == -1)
			{
				m->blocked_stack[i] = Current_Process->pid;
				m->order[i] = m->total_num;
				m->priority_stack[i] = Current_Process->pri;
				break;
			}
		}
		// end of enqueue
		
		
		
		//if cp's priority is higher than the owner
		if (Current_Process->pri < m_owner->pri) {
			m_owner->pri = Current_Process->pri;				// the owner gets cp's priority
		}
		
		//Dispatch();
	}	
}

void Kernel_Unlock_Mutex(void)
{
	MUTEX_TYPE* m = findMutexByMutexID(Current_Process->request_arg);
	PD *m_owner = findProcessByPID(m->owner);
	
	if(m == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Unlock_Mutex: Error finding requested mutex!\n");
		#endif
		return;
	}
	
	if(m->owner != Current_Process->pid)
	{
		#ifdef DEBUG
		printf("Kernel_Unlock_Mutex: The owner is not the current process\n");
		#endif
		
		return;
	} 
	else if (m->count > 1) 
	{
		// M is locked more than once
		--(m->count);
	} 
	else if (m->num_of_process > 0) 
	{
		// there are tasks waiting on the mutex
		// deque the task with highest priority
		PID p_dequeue = 0;
		unsigned int temp_order = m->total_num + 1;
		PRIORITY temp_pri = LOWEST_PRIORITY + 1;
		int i;
		for (i=0; i<MAXTHREAD; i++) 
		{
			if (m->priority_stack[i] < temp_pri) 
			{
				// found a task with higher priority
				temp_pri = m->priority_stack[i];
				temp_order = m->order[i];
				p_dequeue = m->blocked_stack[i];
			} 
			else if (m->priority_stack[i] == temp_pri && temp_order < m->order[i]) 
			{
				// same priority and came into the queue earlier
				temp_order = m->order[i];
				p_dequeue = m->blocked_stack[i];
			}
		}
		
		//dequeue index i
		m->blocked_stack[i] = -1;
		m->priority_stack[i] = LOWEST_PRIORITY+1;
		m->order[i] = 0;
		--(m->num_of_process);
		PD* target_p = findProcessByPID(p_dequeue);
		m_owner->pri = m->own_pri;		//reset owner's priority
		m->owner = p_dequeue;
		m->own_pri = temp_pri;			//keep track of new owner's priority;
		target_p->state = READY;
		Current_Process->state = READY;
		//Dispatch();
		return;
	} 
	else 
	{
		m->owner = 0;
		m->count = 0;
		m_owner->pri = m->own_pri;		//reset owner's priority
		return;
	}
}
