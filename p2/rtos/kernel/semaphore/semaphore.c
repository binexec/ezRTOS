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

SEMAPHORE Kernel_Create_Semaphore(int initial_count, unsigned int is_binary)
{
	int i;
	
	//Ensure There are still free semaphores available
	if (Semaphore_Count >= MAXSEMAPHORE)
	{
		#ifdef DEBUG
		printf("Kernel_Create_Semaphore: Failed to create Semaphore. The system is at its max semaphore threshold.\n");
		#endif
		err = MAX_SEMAPHORE_ERR;
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
	
	return Semaphore[i].id;
}


/************************************************************************/
/*						Semaphore Operations                            */
/************************************************************************/

void Kernel_Semaphore_Get(SEMAPHORE s, unsigned int amount)
{
	int idx = findSemaphoreByID(s);
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
		if(amount > 1)
		{
			amount = 1;
			Current_Process->request_args[1] = 1;
		}
		else if (amount > 0)
			
		{
			amount = 0;
			Current_Process->request_args[1] = 0;
		}
	}	
	
	has_enough = sem->count - amount;
	
	//Are there enough counts in the semaphore to handle this request?
	if(has_enough < 0)				//Putting "sem->count - amount" directly into the if statement doesn't work for some reason
	{
		//printf("Enqueuing...\n");
		
		//Add this process to the semaphore's wait queue
		enqueue(&sem->wait_queue, Current_Process->pid);
		
		//Tell the process to wait
		Current_Process->state = WAIT_SEMAPHORE;
		return;
	}
	
	sem->count -= amount;
	
	//printf("count - amount: %d\n", t);
	//printf("Current GET count: %d\n", sem->count);

}

static inline void Kernel_Semaphore_Get_From_Queue(SEMAPHORE_TYPE *sem, unsigned int amount)
{
	PD *head = findProcessByPID(queue_peek(&sem->wait_queue));	
	
	//See if the semaphore has enough counts to fulfill the amount wanted by the head(s) of the wait queue
	while(sem->count - head->request_args[1] >= 0)
	{
		
		if(head->state != WAIT_SEMAPHORE)
		{
			#ifdef DEBUG
			printf("Kernel_Semaphore_Get_From_Queue: Head of the wait queue isn't waiting for a semaphore!\n");
			#endif
			err = SEMAPHORE_NOT_FOUND_ERR;
			return;
		}
		
		sem->count -= head->request_args[1];
		head->state = READY;
		
		dequeue(&sem->wait_queue);
		head = findProcessByPID(queue_peek(&sem->wait_queue));
	}
}

void Kernel_Semaphore_Give(SEMAPHORE s, unsigned int amount)
{
	int idx = findSemaphoreByID(s);
	SEMAPHORE_TYPE *sem;
	
	if(idx > 0)
	{
		#ifdef DEBUG
		printf("Kernel_Semaphore_Give: The requested Semaphore %d was not found!\n", s);
		#endif
		err = SEMAPHORE_NOT_FOUND_ERR;
		return;
	}
	sem = &Semaphore[idx];
	
	sem->count += amount;
	
	printf("Current count (GIVE): %d\n", sem->count);
	
	//Ensure binary semaphores do not exceed 1 for its count
	if(sem->is_binary && sem->count > 1)
		sem->count = 1;
	
	//Check if any processes are currently waiting for this semaphore, if it's now positive
	if(sem->count > 0 && sem->wait_queue.count > 0)
		Kernel_Semaphore_Get_From_Queue(sem, amount);

}
