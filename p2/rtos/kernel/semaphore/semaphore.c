#include "semaphore.h"


volatile static SEMAPHORE_TYPE Sem[MAXSEMAPHORE];
volatile unsigned int Semaphore_Count;		
volatile unsigned int Last_SemaphoreID;


void Semaphore_Reset(void)
{
	Semaphore_Count = 0;
	Last_SemaphoreID = 0;
	
	memset(Sem, 0, sizeof(SEMAPHORE_TYPE)*MAXSEMAPHORE);
}

/************************************************************************/
/*								Helpers                                 */
/************************************************************************/
int findSemaphoreByID(SEMAPHORE s)
{
	int i;
	
	for(i=0; i<MAXSEMAPHORE; i++)
	{
		if(Sem[i].id == s) 
			return i;
	}
	
	//Semaphore not found
	return -1;
}


/************************************************************************/
/*						Semaphore Creation                              */
/************************************************************************/

static SEMAPHORE Kernel_Create_New_Semaphore(int initial_count, unsigned int is_binary)
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
		if (Sem[i].id == 0) break;
		else continue;
	}
	
	//Create the semaphore object
	Sem[i].id = ++Last_SemaphoreID;
	if(is_binary > 0)
	{
		//Make sure initial_count is either a 1 or 0 for a binary semaphore
		if(initial_count >= 1)
			Sem[i].count = 1;
		else
			Sem[i].count = 0;
		
		Sem[i].is_binary = 1;
	}
	else
	{
		Sem[i].count = initial_count;
		Sem[i].is_binary = 0;
	}
	
	return Sem[i].id;
}

SEMAPHORE Kernel_Create_Semaphore(int initial_count)
{
	return Kernel_Create_New_Semaphore(initial_count, 0);
}

SEMAPHORE Kernel_Create_Binary_Semaphore(int initial_count)
{
	return Kernel_Create_New_Semaphore(initial_count, 1);
}


/************************************************************************/
/*						Semaphore Operations                            */
/************************************************************************/


void Kernel_Semaphore_Give(SEMAPHORE s, unsigned int amount)
{
	int idx = findSemaphoreByID(s);
	
	if(idx > 0)
	{
		#ifdef DEBUG
			printf("Kernel_Semaphore_Give: The requested Semaphore %d was not found!\n", s);
		#endif
		err = SEMAPHORE_NOT_FOUND_ERR;
		return;
	}
	
	Sem[idx].count += amount;
	
	//Check if any processes are currently waiting for this semaphore, if it's now positive
	

}

void Kernel_Semaphore_Get(SEMAPHORE s, unsigned int amount)
{
	
}

