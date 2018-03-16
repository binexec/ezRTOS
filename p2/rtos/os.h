/***********************************************************************
  OS.h and OS.c contains the front end of the RTOS. 
  It contains subroutines on the syscall defined by the requirements. 
  The syscall subroutines do not process the tasks themselves, but make appropriate kernel calls to handle them.
  ***********************************************************************/

#ifndef _OS_H_  
#define _OS_H_  

#include <stdint.h>
#include <stddef.h>

#define DEBUG

	  
/*Task and Memory*/
#define MAXTHREAD					16
#define LOWEST_PRIORITY				10				//0 is the highest priority, 10 the lowest
#define WORKSPACE_HEAP_SIZE			4096			//The total amount of workspace memory dedicated across ALL tasks
#define KERNEL_HEAP_SIZE			1024			//Heap size (in bytes) allocated for the kernel for allocating kernel objects


/*Scheduler configuration*/
//#define PREEMPTIVE_CSWITCH							//Enable preemptive multi-tasking
//#define PREEMPTIVE_CSWITCH_FREQ		25				//How frequently (in ticks) does preemptive scheduling kick in?
#define PREVENT_STARVATION							//Enable starvation prevention in the scheduler
#define STARVATION_MAX				MAXTHREAD*10	//Maximum amount of ticks missed before a task is considered starving


/*Timer*/
#define MSECPERTICK					10				//resolution of a system tick (in milliseconds).


/*Choose which optional kernel modules to enable*/
#define EVENT_ENABLED
#define MUTEX_ENABLED
#define EVENT_GROUP_ENABLED
#define SEMAPHORE_ENABLED
#define MAILBOX_ENABLED





//Identifiers for various RTOS objects. The values are always non-zero if it is valid
typedef unsigned int PID;
typedef unsigned int SEMAPHORE;
typedef unsigned int MUTEX;
typedef unsigned char PRIORITY;
typedef unsigned int EVENT;
typedef unsigned int EVENT_GROUP;
typedef unsigned int MAILBOX;
typedef unsigned int TICK;

typedef void (*taskfuncptr) (void);      /* pointer to void f(void), used to represent the main function for a RTOS task */

#ifndef NULL
#define NULL          0					/* undefined */
#endif


//Identifiers for various RTOS objects. The values are always non-zero if it is valid
typedef unsigned int PID; 
typedef unsigned int SEMAPHORE;
typedef unsigned int MUTEX;
typedef unsigned char PRIORITY;
typedef unsigned int EVENT;
typedef unsigned int TICK;


/*OS Initialization*/
void OS_Init(void);
void OS_Start(void);
void OS_Abort(void);



/*Task/Thread related functions*/
PID Task_Create(taskfuncptr f, size_t stack_size, PRIORITY py, int arg);
void Task_Terminate(void);
void Task_Sleep(TICK t);													// sleep time is at least t*MSECPERTICK
void Task_Suspend(PID p);													//Suspend/Resume tasks by the function name instead
void Task_Resume(PID p);
void Task_Yield(void);
int  Task_GetArg(void);




/*Mutex related functions*/
#ifdef MUTEX_ENABLED
MUTEX Mutex_Create(void);
int Mutex_Destroy(MUTEX m);
void Mutex_Lock(MUTEX m);
void Mutex_Unlock(MUTEX m);
#endif


/*EVENT related functions*/
#ifdef EVENT_ENABLED
EVENT Event_Create(void);
void Event_Wait(EVENT e);
void Event_Signal(EVENT e);
#endif


/*SEMAPHORE related functions*/
#ifdef SEMAPHORE_ENABLED
SEMAPHORE Semaphore_Create(int initial_count, unsigned int is_binary);
int Semaphore_Destroy(SEMAPHORE s);
void Semaphore_Give(SEMAPHORE s, unsigned int amount);
void Semaphore_Get(SEMAPHORE s, unsigned int amount);
#endif


/*EVENT GROUP related functions*/
#ifdef EVENT_GROUP_ENABLED
EVENT_GROUP Event_Group_Create(void);
int Event_Group_Destroy(EVENT_GROUP eg);
void Event_Group_Set_Bits(EVENT_GROUP e, unsigned int bits_to_set);
void Event_Group_Clear_Bits(EVENT_GROUP e, unsigned int bits_to_clear);
void Event_Group_Wait_Bits(EVENT_GROUP e, unsigned int bits_to_wait, unsigned int wait_all_bits, TICK timeout);
#endif


/*MAILBOX*/
#ifdef MAILBOX_ENABLED
typedef struct MAIL MAIL;													//Formally declared in mailbox/mailbox.h

MAILBOX Mailbox_Create(unsigned int capacity);
void Mailbox_Destroy(MAILBOX mb);
int Mailbox_Check_Mail(MAILBOX mb);
int Mailbox_Send_Mail(MAILBOX mb, MAIL* m);
int Mailbox_Get_Mail(MAILBOX mb, MAIL* received);
int Mailbox_Wait_Mail(MAILBOX mb, MAIL* received, TICK timeout);
#endif 


#endif /* _OS_H_ */
