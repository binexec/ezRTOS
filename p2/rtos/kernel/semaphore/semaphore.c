#include "semaphore.h"
#include <stdlib.h>		//Remove once kmalloc is used


volatile static PtrList SemaphoreList;
volatile unsigned int Semaphore_Count;		
volatile unsigned int Last_SemaphoreID;


void Semaphore_Reset(void)
{
	Semaphore_Count = 0;
	Last_SemaphoreID = 0;
	
	SemaphoreList.ptr = NULL;
	SemaphoreList.next = NULL;
}

/************************************************************************/
/*								Helpers                                 */
/************************************************************************/
SEMAPHORE_TYPE* findSemaphoreByID(SEMAPHORE s)
{
	PtrList *i;
	SEMAPHORE_TYPE *sem_i;
	
	for(i = &SemaphoreList; i; i = i->next)
	{
		sem_i = (SEMAPHORE_TYPE*)i->ptr;
		if (sem_i->id == s)
			return sem_i;
	}
	
	//Semaphore not found
	return NULL;
}


/************************************************************************/
/*						Semaphore Creation                              */
/************************************************************************/

//Create a semaphore before the kernel was active
SEMAPHORE Kernel_Create_Semaphore_Direct(int initial_count, unsigned int is_binary)
{
	SEMAPHORE_TYPE *sem;
	
	//Ensure There are still free semaphores available
	if (Semaphore_Count >= MAXSEMAPHORE)
	{
		#ifdef DEBUG
		printf("Kernel_Create_Semaphore: Failed to create Semaphore. The system is at its max semaphore threshold.\n");
		#endif
		
		kernel_raise_error(MAX_OBJECT_ERR);
		return 0;
	}
	
	//Create a new Semaphore object and add it to the semaphore list
	sem = malloc(sizeof(SEMAPHORE_TYPE));
	ptrlist_add(&SemaphoreList, sem);
	++Semaphore_Count;

	sem->id = ++Last_SemaphoreID;
	sem->wait_queue = new_pid_queue();

	//Creating a binary semaphore
	if(is_binary > 0)
	{
		sem->is_binary = 1;
		
		//Make sure initial_count is either a 1 or 0 for a binary semaphore
		if(initial_count >= 1)
			sem->count = 1;
		else
			sem->count = 0;
	}
	//Creating a counting semaphore
	else
	{
		sem->is_binary = 0;
		sem->count = initial_count;
	}
	
	
	
	return sem->id;
}


void Kernel_Create_Semaphore()
{
	#define req_initial_count		Current_Process->request_args[0].val
	#define req_is_binary			Current_Process->request_args[1].val
	
	Current_Process->request_retval = Kernel_Create_Semaphore_Direct(req_initial_count, req_is_binary);
	
	#undef req_initial_count
	#undef req_is_binary
}


void Kernel_Destroy_Semaphore()
{
	#define req_sem_id		Current_Process->request_args[0].val
	
	PtrList *i;
	SEMAPHORE_TYPE *sem;	
	
	//Find the corresponding Semaphore object in the semaphore list
	for(i = &SemaphoreList; i; i = i->next)
	{
		sem = (SEMAPHORE_TYPE*)i->ptr;
		if (sem->id == req_sem_id)
			break;
	}

	if(!sem)
	{
		#ifdef DEBUG
		printf("Kernel_Destroy_Semaphore: The requested Semaphore %d was not found!\n", req_sem_id);
		#endif
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		return;
	}
	
	/*Should we check and make sure the wait queue is empty first?*/
	
	free(sem);
	ptrlist_remove(&SemaphoreList, i);
	--Semaphore_Count;
	
	
	#undef req_sem_id
}


/************************************************************************/
/*						Semaphore Operations                            */
/************************************************************************/

static inline void Kernel_Semaphore_Get_From_Queue(SEMAPHORE_TYPE *sem)
{
	#define head_req_amount		head->request_args[1].val
	
	PD *head = findProcessByPID(queue_peek_pid(&sem->wait_queue));	
	
	//See if the semaphore has enough counts to fulfill the amount wanted by the head(s) of the wait queue
	while(sem->count - head_req_amount >= 0)
	{
		if(head->state != WAIT_SEMAPHORE)
		{
			#ifdef DEBUG
			printf("Kernel_Semaphore_Get_From_Queue: Head of the wait queue isn't waiting for a semaphore!\n");
			#endif
			kernel_raise_error(UNPROCESSABLE_TASK_STATE_ERR);
			return;
		}
		
		sem->count -= head_req_amount;
		head->state = READY;
		
		dequeue_pid(&sem->wait_queue);
		head = findProcessByPID(queue_peek_pid(&sem->wait_queue));
	}
	
	#undef head_req_amount
}


void Kernel_Semaphore_Get()
{
	#define req_sem_id		Current_Process->request_args[0].val
	#define req_amount		Current_Process->request_args[1].val
	
	SEMAPHORE_TYPE *sem = findSemaphoreByID(req_sem_id);
	int has_enough;
	
	if(!sem)
	{
		#ifdef DEBUG
		printf("Kernel_Semaphore_Get: The requested Semaphore %d was not found!\n", req_sem_id);
		#endif
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		return;
	}
	
	//If it's a binary semaphore, do not allow more than 1 counts to be taken
	if(sem->is_binary)
	{
		if(req_amount > 1)
		req_amount = 1;
		else if(req_amount < 0)
		req_amount = 0;
	}
	
	//Are there enough counts in the semaphore to handle this request?
	has_enough = sem->count - req_amount;
	
	//If not, add the process to the semaphore's waiting queue, and put the task into the WAIT_SEMAPHORE state
	if(has_enough < 0)
	{
		enqueue_pid(&sem->wait_queue, Current_Process->pid);
		Current_Process->state = WAIT_SEMAPHORE;
		Kernel_Request_Cswitch = 1;
		return;
	}
	
	sem->count -= req_amount;

	#undef req_sem_id
	#undef req_amount
}


void Kernel_Semaphore_Give()
{
	#define req_sem_id		Current_Process->request_args[0].val
	#define req_amount		Current_Process->request_args[1].val
	
	SEMAPHORE_TYPE *sem = findSemaphoreByID(req_sem_id);
	
	if(!sem)
	{
		#ifdef DEBUG
		printf("Kernel_Semaphore_Give: The requested Semaphore %d was not found!\n", req_sem_id);
		#endif
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		return;
	}
	
	sem->count += req_amount;

	//Ensure binary semaphores do not exceed 1 for its count
	if(sem->is_binary && sem->count > 1)
		sem->count = 1;
	
	//Check if any processes are currently waiting for this semaphore, if it's now positive
	if(sem->count > 0 && sem->wait_queue.count > 0)
		Kernel_Semaphore_Get_From_Queue(sem);
		
	#undef req_sem_id
	#undef req_amount
}
