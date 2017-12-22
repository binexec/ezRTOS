#include "semaphore.h"


volatile static SEMAPHORE_TYPE Semaphore[MAXSEMAPHORE];
volatile unsigned int Semaphore_Count;		
volatile unsigned int Last_SemaphoreID;


void Semaphore_Reset(void)
{
	Semaphore_Count = 0;
	Last_SemaphoreID = 0;
	
	memset(Semaphore, 0, sizeof(SEMAPHORE_TYPE)*MAXSEMAPHORE);
}

/************************************************************************/
/*								Helpers                                 */
/************************************************************************/
int findSemaphoreByID(SEMAPHORE s)
{
	int i;
	
	for(i=0; i<MAXSEMAPHORE; i++)
	{
		if(Semaphore[i].id == s) 
			return i;
	}
	
	//Semaphore not found
	return -1;
}


/************************************************************************/
/*						Semaphore Creation                              */
/************************************************************************/

SEMAPHORE Kernel_Create_Semaphore()
{
	#define req_initial_count		Current_Process->request_args[0]
	#define req_is_binary			Current_Process->request_args[1]
	
	return Kernel_Create_Semaphore_Direct(req_initial_count, req_is_binary);
	
	#undef req_initial_count
	#undef req_is_binary
}

//Create a semaphore before the kernel was active
SEMAPHORE Kernel_Create_Semaphore_Direct(int initial_count, unsigned int is_binary)
{
	int i;
	
	//Ensure There are still free semaphores available
	if (Semaphore_Count >= MAXSEMAPHORE)
	{
		#ifdef DEBUG
		printf("Kernel_Create_Semaphore: Failed to create Semaphore. The system is at its max semaphore threshold.\n");
		#endif
		
		err = MAX_SEMAPHORE_ERR;
		if(KernelActive)
			Current_Process->request_ret = 0;
		return 0;
	}
	
	//Find a free slot for the semaphore
	for(i=0; i<MAXSEMAPHORE; i++)
	{
		if (Semaphore[i].id == 0)
			break;
	}
	
	//Creating a binary semaphore
	if(is_binary > 0)
	{
		//Make sure initial_count is either a 1 or 0 for a binary semaphore
		if(initial_count >= 1)
			Semaphore[i].count = 1;
		else
			Semaphore[i].count = 0;
		
		Semaphore[i].is_binary = 1;
	}
	//Creating a counting semaphore
	else
	{
		Semaphore[i].count = initial_count;
		Semaphore[i].is_binary = 0;
	}
	
	Semaphore[i].id = ++Last_SemaphoreID;
	Semaphore[i].wait_queue = new_queue();
	++Semaphore_Count;
	
	err = NO_ERR;
	if(KernelActive)
		Current_Process->request_ret = Semaphore[i].id;
	return Semaphore[i].id;
}


/************************************************************************/
/*						Semaphore Operations                            */
/************************************************************************/

void Kernel_Semaphore_Get()
{
	#define req_sem_id		Current_Process->request_args[0]
	#define req_amount		Current_Process->request_args[1]
	
	int idx = findSemaphoreByID(req_sem_id);
	int has_enough;
	SEMAPHORE_TYPE *sem;
	
	if(idx < 0)
	{
		#ifdef DEBUG
		printf("Kernel_Semaphore_Get: The requested Semaphore %d was not found!\n", s);
		#endif
		err = SEMAPHORE_NOT_FOUND_ERR;
		return;
	}
	sem = &Semaphore[idx];
	
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
		enqueue(&sem->wait_queue, Current_Process->pid);
		Current_Process->state = WAIT_SEMAPHORE;
		Kernel_Request_Cswitch = 1;
		return;
	}
	
	sem->count -= req_amount;

	#undef req_sem_id	
	#undef req_amount	
}

static inline void Kernel_Semaphore_Get_From_Queue(SEMAPHORE_TYPE *sem, unsigned int amount)
{
	PD *head = findProcessByPID(queue_peek(&sem->wait_queue));	
	
	#define head_req_amount		head->request_args[1]
	
	//See if the semaphore has enough counts to fulfill the amount wanted by the head(s) of the wait queue
	while(sem->count - head_req_amount >= 0)
	{
		if(head->state != WAIT_SEMAPHORE)
		{
			#ifdef DEBUG
			printf("Kernel_Semaphore_Get_From_Queue: Head of the wait queue isn't waiting for a semaphore!\n");
			#endif
			err = SEMAPHORE_NOT_FOUND_ERR;
			return;
		}
		
		sem->count -= head_req_amount;
		head->state = READY;
		
		dequeue(&sem->wait_queue);
		head = findProcessByPID(queue_peek(&sem->wait_queue));
	}
	
	#undef head_req_amount
}

void Kernel_Semaphore_Give()
{
	#define req_sem_id		Current_Process->request_args[0]
	#define req_amount		Current_Process->request_args[1]
	
	int idx = findSemaphoreByID(req_sem_id);
	SEMAPHORE_TYPE *sem;
	
	if(idx > 0)
	{
		#ifdef DEBUG
		printf("Kernel_Semaphore_Give: The requested Semaphore %d was not found!\n", req_sem_id);
		#endif
		err = SEMAPHORE_NOT_FOUND_ERR;
		return;
	}
	
	sem = &Semaphore[idx];
	sem->count += req_amount;

	//Ensure binary semaphores do not exceed 1 for its count
	if(sem->is_binary && sem->count > 1)
		sem->count = 1;
	
	//Check if any processes are currently waiting for this semaphore, if it's now positive
	if(sem->count > 0 && sem->wait_queue.count > 0)
		Kernel_Semaphore_Get_From_Queue(sem, req_amount);
		
	#undef req_sem_id
	#undef req_amount
}
