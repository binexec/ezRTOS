#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include "../kernel_shared.h"
#include "../others/Queue.h"


#define MAXSEMAPHORE				8


typedef struct {
	
	SEMAPHORE id;
	int count;					
	unsigned int is_binary;				//0 if it's a counting semaphore; 1 if it's a binary semaphore
	Queue wait_queue;

} SEMAPHORE_TYPE;


/*Variables Accessible by the OS*/
extern volatile unsigned int Semaphore_Count;		//Number of Mutexes created so far.
extern volatile unsigned int Last_SemaphoreID;


void Kernel_Create_Semaphore(void);
SEMAPHORE Kernel_Create_Semaphore_Direct(int initial_count, unsigned int is_binary);
void Kernel_Destroy_Semaphore(void);
void Semaphore_Reset(void);
void Kernel_Semaphore_Give(void);
void Kernel_Semaphore_Get(void);



#endif /* SEMAPHORE_H_ */