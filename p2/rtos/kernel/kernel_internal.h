/*
 * Kernel variables and declarations that are used among all other kernel modules
 */ 

#ifndef KERNEL_INTERNAL_H_
#define KERNEL_INTERNAL_H_

#include "../os.h"
#include <string.h>
#include <stdint.h>

  
  //Definitions for potential errors the RTOS may come across
  typedef enum error_codes
  {
	  NO_ERR  = 0,
	  INVALID_ARG_ERR,
	  INVALID_KERNET_REQUEST_ERR,
	  KERNEL_INACTIVE_ERR,
	  MAX_PROCESS_ERR,
	  PID_NOT_FOUND_ERR,
	  SUSPEND_NONRUNNING_TASK_ERR,
	  RESUME_NONSUSPENDED_TASK_ERR,
	  MAX_EVENT_ERR,
	  EVENT_NOT_FOUND_ERR,
	  EVENT_ALREADY_OWNED_ERR,
	  SIGNAL_UNOWNED_EVENT_ERR,
	  MAX_MUTEX_ERR,
	  MUTEX_NOT_FOUND_ERR,
	  MAX_SEMAPHORE_ERR,
	  SEMAPHORE_NOT_FOUND_ERR
  } ERROR_TYPE;

  
  typedef enum process_states
  {
	  DEAD = 0,
	  READY,
	  RUNNING,
	  SUSPENDED,
	  SLEEPING,
	  WAIT_EVENT,
	  WAIT_MUTEX
  } PROCESS_STATES;


  typedef enum kernel_request_type
  {
	  NONE = 0,
	  CREATE_T,								//Create a task
	  YIELD,
	  TERMINATE,
	  SUSPEND,
	  RESUME,
	  SLEEP,
	  CREATE_E,							//Initialize an event object
	  WAIT_E,
	  SIGNAL_E,
	  CREATE_M,							//Initialize a mutex object
	  LOCK_M,
	  UNLOCK_M
  } KERNEL_REQUEST_TYPE;


/*Process descriptor for a task*/
typedef struct ProcessDescriptor
{
	PID pid;									//An unique process ID for this task.
	PRIORITY pri;							//The priority of this task, from 0 (highest) to 10 (lowest).
	PROCESS_STATES state;					//What's the current state of this task?
	PROCESS_STATES last_state;				//What's the PREVIOUS state of this task? Used for task suspension/resume.
	KERNEL_REQUEST_TYPE request;				//What the task want the kernel to do (when needed).
	int request_arg;							//What value is needed for the specified kernel request.
	int arg;									//Initial argument for the task (if specified).
	unsigned char *sp;						//stack pointer into the "workSpace".
	unsigned char workSpace[WORKSPACE];		//Data memory allocated to this process.
	voidfuncptr  code;						//The function to be executed when this process is running.
} PD;


/*Shared Variables*/
//extern volatile PD Process[MAXTHREAD];			//Contains the process descriptor for all tasks, regardless of their current state.
extern volatile PD* Current_Process;					//Process descriptor for the last running process before entering the kernel
extern volatile ERROR_TYPE err;						//Error code for the previous kernel operation (if any)

/*Shared functions*/
PD* findProcessByPID(int pid);


#endif /* KERNEL_INTERNAL_H_ */