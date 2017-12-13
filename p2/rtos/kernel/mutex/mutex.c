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
/*						HELPER FUNCTIONS	                            */
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



static PRIORITY findHighestFromQueue(PID_Queue *q)
{
	int i;
	PRIORITY current;
	PRIORITY highest = iterate_queue(q);
	
	for(i=0; i < q->count-2; i++)
	{
		current = iterate_queue(NULL);
		if(highest < current)
			highest = current;
	}
	
	return highest;
}

static void applyNewPriorityToWaitQueue(PID_Queue *q, PRIORITY pri)
{
	int i;
	PD *p = findProcessByPID(iterate_queue(q));
	
	for(i=0; i < q->count-1; i++)
	{
		p->pri = pri;
		p = findProcessByPID(iterate_queue(NULL));
	}
}

/************************************************************************/
/*							MUTEX Operations		                    */
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
	
	
	Mutex[i].id = ++Last_MutexID;
	Mutex[i].owner = 0;		
	Mutex[i].lock_count = 0;
	Mutex[i].highest_priority = LOWEST_PRIORITY;
	Mutex[i].wait_queue = new_queue();
	Mutex[i].orig_priority = new_queue();
	
	#ifdef DEBUG
	printf("Kernel_Create_Mutex: Created Mutex %d!\n", Last_MutexID);
	#endif

}



void Kernel_Lock_Mutex(void)
{
	MUTEX_TYPE *m = findMutexByMutexID(Current_Process->request_args[0]);
	
	if(m == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Lock_Mutex: Error finding requested mutex!\n");
		#endif
		return;
	}
	
	//Mutex is unowned: lock mutex 
	if(m->owner == 0)
	{
		m->owner = Current_Process->pid;
		m->owner_orig_priority = Current_Process->pri;
		m->highest_priority = Current_Process->pri;
		++m->lock_count;
		return;
	}
	
	//If I'm already the owner: recursive lock
	if(m->owner == Current_Process->pid)
	{
		++m->lock_count;
		return;
	}
		
	//If I'm not the owner (mutex already locked): Add the current process to the wait queue
	Current_Process->state = WAIT_MUTEX;
	enqueue(&m->wait_queue, Current_Process->pid);
	enqueue(&m->orig_priority, Current_Process->pri);
	
	//Inherit the highest priority if mine's not the highest
	if(m->highest_priority < Current_Process->pri)
		Current_Process->pri = m->highest_priority;
		
	//Let all other tasks waiting for the same mutex inherit my priority, since I have the highest
	else if(Current_Process->pri < m->highest_priority)
		m->highest_priority = Current_Process->pri;	
	
}



static void Kernel_Lock_Mutex_From_Queue(MUTEX_TYPE *m)
{
	PD *p;
	
	//Pass the mutex to the head of the wait queue and lock it
	m->owner = dequeue(&m->wait_queue);
	m->owner_orig_priority = dequeue(&m->orig_priority);
	m->lock_count++;

	//Wake up the new mutex owner from its waiting state	
	p = findProcessByPID(m->owner);
	
	if(p->state != WAIT_MUTEX)
	{
		printf("PID %d IS NOT IN WAIT_MUTEX\n", p->pid);
		return;
	}
	
	p->state = READY;
	p->pri = m->highest_priority;		//Inherit the highest priority when entering the task's critical section
	
	//Tell the kernel to switch to another task if there are others waiting on this mutex
	Current_Process->state = READY;
	Current_Process->request = YIELD;
	
}



void Kernel_Unlock_Mutex(void)
{
	MUTEX_TYPE* m = findMutexByMutexID(Current_Process->request_args[0]);
	
	if(m == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Unlock_Mutex: Error finding requested mutex!\n");
		#endif
		return;
	}
	
	//Only the mutex owner can unlock the mutex
	if(m->owner != Current_Process->pid)
	{
		#ifdef DEBUG
		printf("Kernel_Unlock_Mutex: Mutex was attempted to be unlocked not by its owner!\n");
		#endif
		err = MUTEX_NOT_FOUND_ERR;
		return;
	}
	
	//Decrement the lock if it's recursively locked
	--m->lock_count;
	if(m->lock_count > 0)
	{
		return;
	}
		
	//If not recursively locked, the current owner will now give up the mutex (with its original priority restored in its PD)
	Current_Process->pri = m->owner_orig_priority;		
	m->lock_count = 0;
	
	//If there is no one else waiting to lock this mutex, leave it unlocked and unowned
	if(m->wait_queue.count == 0)
	{
		m->owner = 0;
		return;
	}

	//If there are other tasks waiting for the mutex
	Kernel_Lock_Mutex_From_Queue(m);
}
