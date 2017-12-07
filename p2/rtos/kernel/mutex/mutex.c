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
	


	
	#ifdef DEBUG
	printf("Kernel_Create_Mutex: Created Mutex %d!\n", Last_MutexID);
	#endif

}



void Kernel_Lock_Mutex(void)
{
	MUTEX_TYPE* m = findMutexByMutexID(Current_Process->request_args[0]);
	PD *m_owner = findProcessByPID(m->owner);
	
	if(m == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Lock_Mutex: Error finding requested mutex!\n");
		#endif
		return;
	}
	
}

void Kernel_Unlock_Mutex(void)
{
	MUTEX_TYPE* m = findMutexByMutexID(Current_Process->request_args[0]);
	PD *m_owner = findProcessByPID(m->owner);
	
	if(m == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Unlock_Mutex: Error finding requested mutex!\n");
		#endif
		return;
	}
	
}
