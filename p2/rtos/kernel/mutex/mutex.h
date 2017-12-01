#ifndef MUTEX_H_
#define MUTEX_H_

#include "../kernel_internal.h"

//For the ease of manageability, we're making a new mutex data type. The old MUTEX type defined in OS.h will simply serve as an identifier.
typedef struct mutex_type
{
	MUTEX id;								//unique id for this mutex, 0 = uninitialized
	PID owner;								//the current owner of the event, 0 = free
	unsigned int count;						//mutex can be recursively locked
	PID blocked_stack [MAXTHREAD];			//stack for blocked
	PRIORITY priority_stack [MAXTHREAD];	//priority of the processes
	unsigned int order[MAXTHREAD];			//order of task came into the stack
	unsigned int num_of_process;			//number of processes waiting on the mutex
	unsigned int total_num;					//total number of process has waitted on this mutex
	PRIORITY own_pri;						//original priority of the owner
} MUTEX_TYPE;

/*Variables Accessible by the OS*/
extern volatile unsigned int Mutex_Count;		//Number of Mutexes created so far.
extern volatile unsigned int Last_MutexID;

/*Accessible by OS*/
MUTEX_TYPE* findMutexByMutexID(MUTEX m);
void Kernel_Create_Mutex(void);

/*Accessible within kernel only*/
void Mutex_Reset(void);
void Kernel_Lock_Mutex(void);
void Kernel_Unlock_Mutex(void);



#endif /* MUTEX_H_ */