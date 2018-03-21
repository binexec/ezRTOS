#include "mutex.h"
#include <string.h>
#include <stdlib.h>		//Remove once kmalloc is used

volatile static PtrList MutexList;				//Contains all the mutex objects
volatile unsigned int Mutex_Count;				//Number of Mutexes created so far.
volatile unsigned int Last_MutexID;				//Last (also highest) MUTEX value created so far.

/************************************************************************/
/*						USED DURING BOOTING                             */
/************************************************************************/

void Mutex_Reset()
{
	Mutex_Count = 0;
	Last_MutexID = 0;
	
	MutexList.ptr = NULL;
	MutexList.next = NULL;
}

/************************************************************************/
/*						HELPER FUNCTIONS	                            */
/************************************************************************/

MUTEX_TYPE* findMutexByMutexID(MUTEX m)
{
	PtrList *i;
	MUTEX_TYPE *mutex_i;
	
	//Ensure the request mutex ID is > 0
	if(m <= 0)
	{
		#ifdef DEBUG
		printf("findMutexByID: The specified mutex ID is invalid!\n");
		#endif
		kernel_raise_error(INVALID_ARG_ERR);
		return NULL;
	}
	
	for(i = &MutexList; i; i = i->next)
	{
		mutex_i = (MUTEX_TYPE*)i->ptr;
		if (mutex_i->id == m)
			return mutex_i;
	}
	
	kernel_raise_error(OBJECT_NOT_FOUND_ERR);
	return NULL;
}


/************************************************************************/
/*							MUTEX Creation 			                    */
/************************************************************************/

//We don't need a Kernel_Create_Mutex_Direct function, since no arguments are needed to create a new mutex object

MUTEX Kernel_Create_Mutex(void)
{
	MUTEX_TYPE *mut;
	
	//Make sure the system's mutexes are not at max
	if(Mutex_Count >= MAXMUTEX)
	{
		#ifdef DEBUG
		printf("Kernel_Create_Mutex: Failed to create Mutex. The system is at its max mutex threshold.\n");
		#endif
		
		kernel_raise_error(MAX_OBJECT_ERR);
		if(KernelActive)
			Current_Process->request_retval = 0;
		return 0;
	}
	
	//Create a new Mutex object
	mut = malloc(sizeof(MUTEX_TYPE));
	ptrlist_add(&MutexList, mut);
	++Mutex_Count; 
	
	mut->id = ++Last_MutexID;
	mut->owner = 0;		
	mut->lock_count = 0;
	mut->highest_priority = LOWEST_PRIORITY;
	mut->wait_queue = new_ptr_queue();
	mut->orig_priority = new_int_queue();
	
	#ifdef DEBUG
	printf("Kernel_Create_Mutex: Created Mutex %d!\n", Last_MutexID);
	#endif
	
	
	if(KernelActive)
		Current_Process->request_retval = mut->id;
	return mut->id;
}


void Kernel_Destroy_Mutex(void)
{
	#define req_mut_id		Current_Process->request_args[0].val
	
	PtrList *i;
	MUTEX_TYPE *mut;
	
	//Find the corresponding Semaphore object in the semaphore list
	for(i = &MutexList; i; i = i->next)
	{
		mut = (MUTEX_TYPE*)i->ptr;
		if (mut->id == req_mut_id)
			break;
	}

	if(!mut)
	{
		#ifdef DEBUG
		printf("Kernel_Destroy_Mutex: The requested Mutex %d was not found!\n", req_mut_id);
		#endif
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		return;
	}
	
	/*Should we check and make sure the wait queue is empty first?*/
	
	free(mut);
	ptrlist_remove(&MutexList, i);
	--Mutex_Count;
	
	
	#undef req_mut_id
}



/************************************************************************/
/*							MUTEX Operations		                    */
/************************************************************************/


void Kernel_Lock_Mutex(void)
{
	#define req_mut_id		Current_Process->request_args[0].val
	
	MUTEX_TYPE *m = findMutexByMutexID(req_mut_id);
	
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
	enqueue_ptr(&m->wait_queue, Current_Process);
	enqueue_int(&m->orig_priority, Current_Process->pri);
	
	//Inherit the highest priority if mine's not the highest
	if(m->highest_priority < Current_Process->pri)
		Current_Process->pri = m->highest_priority;
		
	//Let all other tasks waiting for the same mutex inherit my priority, since I have the highest
	else if(Current_Process->pri < m->highest_priority)
		m->highest_priority = Current_Process->pri;	
	
	//Put myself to the wait state
	Current_Process->state = WAIT_MUTEX;
	Kernel_Request_Cswitch = 1;
	
	#undef req_mut_id
}



static void Kernel_Lock_Mutex_From_Queue(MUTEX_TYPE *m)
{
	PD *p = dequeue_ptr(&m->wait_queue);
	
	//Pass the mutex to the head of the wait queue and lock it
	m->owner = p->pid;
	m->owner_orig_priority = dequeue_int(&m->orig_priority);
	m->lock_count++;

	//Wake up the new mutex owner from its waiting state		
	if(p->state != WAIT_MUTEX)
	{
		printf("PID %d IS NOT IN WAIT_MUTEX\n", p->pid);
		return;
	}
	
	p->state = READY;
	p->pri = m->highest_priority;		//Inherit the highest priority when entering the task's critical section
	
	//Tell the kernel to switch to another task if there are others waiting on this mutex
	Current_Process->state = READY;
	Kernel_Request_Cswitch = 1;
}



void Kernel_Unlock_Mutex(void)
{
	#define req_mut_id		Current_Process->request_args[0].val
	
	MUTEX_TYPE* m = findMutexByMutexID(req_mut_id);
	
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
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
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
	
	#undef req_mut_id
}
