/***********************************************************************
  OS.h and OS.c contains the front end of the RTOS. 
  It contains subroutines on the syscall defined by the requirements. 
  The syscall subroutines do not process the tasks themselves, but make appropriate kernel calls to handle them.
  ***********************************************************************/

#ifndef _OS_H_  
#define _OS_H_  
      
	  
/*Task*/
#define MAXTHREAD					16
#define LOWEST_PRIORITY				10		// 0 is the highest priority, 10 the lowest
#define WORKSPACE					256		// in bytes, per THREAD
#define KERNEL_HEAP_SIZE			1024

/*Optional Kernel Modules*/
#define MAXMUTEX					8
#define MAXSEMAPHORE				8
#define MAXEVENT					8
#define MAXEVENTGROUP				8
#define MAXMAILBOX					8

/*Timer*/
#define MSECPERTICK					10		// resolution of a system tick in milliseconds

/*Scheduler configuration*/
#define MAX_TICK_MISSED				10
//#define PREEMPTIVE_CSWITCH							//Enable preemptive multi-tasking
//#define PREEMPTIVE_CSWITCH_FREQ		25				//How frequently (in ticks) does preemptive scheduling kick in?
//#define PREVENT_STARVATION							//Enable starvation prevention in the scheduler
//#define STARVATION_MAX				MAXTHREAD*10	//Maximum amount of ticks missed before a task is considered starving


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
PID  Task_Create(taskfuncptr f, PRIORITY py, int arg);
void Task_Suspend(taskfuncptr f);		//Suspend/Resume tasks by the function name instead
void Task_Resume(taskfuncptr f);
void Task_Terminate(void);
void Task_Yield(void);
int  Task_GetArg(void);
void Task_Sleep(TICK t);		// sleep time is at least t*MSECPERTICK


/*Mutex related functions*/
MUTEX Mutex_Init(void);
void Mutex_Lock(MUTEX m);
void Mutex_Unlock(MUTEX m);


/*EVENT related functions*/
EVENT Event_Init(void);
void Event_Wait(EVENT e);
void Event_Signal(EVENT e);


/*SEMAPHORE related functions*/
SEMAPHORE Semaphore_Init(int initial_count, unsigned int is_binary);
void Semaphore_Give(SEMAPHORE s, unsigned int amount);
void Semaphore_Get(SEMAPHORE s, unsigned int amount);


/*EVENT GROUP related functions*/
EVENT_GROUP Event_Group_Init();
void Event_Group_Set_Bits(EVENT_GROUP e, unsigned int bits_to_set);
void Event_Group_Clear_Bits(EVENT_GROUP e, unsigned int bits_to_clear);
void Event_Group_Wait_Bits(EVENT_GROUP e, unsigned int bits_to_wait, unsigned int wait_all_bits, TICK timeout);


#endif /* _OS_H_ */
