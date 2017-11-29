/***********************************************************************
  OS.h and OS.c contains the front end of the RTOS. 
  It contains subroutines on the syscall defined by the requirements. 
  The syscall subroutines do not process the tasks themselves, but make appropriate kernel calls to handle them.
  ***********************************************************************/

#ifndef _OS_H_  
#define _OS_H_  
   
#define MAXTHREAD     16       
#define WORKSPACE     256   // in bytes, per THREAD
#define MAXMUTEX      8 
#define MAXEVENT      8      
#define MSECPERTICK   10   // resolution of a system tick in milliseconds
#define MINPRIORITY   10   // 0 is the highest priority, 10 the lowest

typedef void (*voidfuncptr) (void);      /* pointer to void f(void) */

#ifndef NULL
	#define NULL          0   /* undefined */
#endif

typedef unsigned int PID;        // always non-zero if it is valid
typedef unsigned int MUTEX;      // always non-zero if it is valid
typedef unsigned char PRIORITY;
typedef unsigned int EVENT;      // always non-zero if it is valid
typedef unsigned int TICK;

/*OS Initialization*/
void OS_Init(void);
void OS_Start(void);
void OS_Abort(void);

/*Task/Thread related functions*/
PID  Task_Create(voidfuncptr f, PRIORITY py, int arg);
void Task_Terminate(void);
void Task_Yield(void);
int  Task_GetArg(void);
void Task_Suspend( PID p );          
void Task_Resume( PID p );
void Task_Sleep(TICK t);		// sleep time is at least t*MSECPERTICK

/*Mutex related functions*/
MUTEX Mutex_Init(void);
void Mutex_Lock(MUTEX m);
void Mutex_Unlock(MUTEX m);

/*EVENT related functions*/
EVENT Event_Init(void);
void Event_Wait(EVENT e);
void Event_Signal(EVENT e);

#endif /* _OS_H_ */
