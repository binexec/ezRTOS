/*
 * Kernel variables and declarations that are used among all other kernel modules
 */ 

#ifndef KERNEL_INTERNAL_H_
#define KERNEL_INTERNAL_H_

#include "../os.h"			//will also include kernel.h
#include "others/PtrList.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>


#define MAX_KERNEL_ARGS		5


/************************************************************************/
/*					KERNEL DATA TYPE DECLARATIONS                       */
/************************************************************************/
  
 /*Definitions for potential errors the RTOS may come across*/
  typedef enum 
  {
	  NO_ERR  = 0,
	  INVALID_ARG_ERR,
	  INVALID_KERNET_REQUEST_ERR,
	  KERNEL_INACTIVE_ERR,
	  MAX_PROCESS_ERR,
	  PID_NOT_FOUND_ERR,
	  UNSUSPENDABLE_TASK_STATE_ERR,
	  RESUME_NONSUSPENDED_TASK_ERR,
	  MAX_EVENT_ERR,
	  EVENT_NOT_FOUND_ERR,
	  EVENT_ALREADY_OWNED_ERR,
	  MAX_MUTEX_ERR,
	  MUTEX_NOT_FOUND_ERR,
	  MAX_SEMAPHORE_ERR,
	  SEMAPHORE_NOT_FOUND_ERR,
	  MAX_EVENTG_ERR,
	  EVENTG_NOT_FOUND_ERR
  } ERROR_CODE;

  /*Definitions for all possible states a process can be in*/
   typedef enum
   {
	   DEAD = 0,
	   READY,
	   RUNNING,
	   SUSPENDED,
	   SLEEPING,
	   WAIT_EVENT,
	   WAIT_EVENTG,
	   WAIT_MUTEX,
	   WAIT_SEMAPHORE
   } PROCESS_STATE;


/*Definitions for all available kernel requests.*/
/*The numerical value for each request is equivalent to its "kernel call code"*/
   typedef enum 
   {
	   NONE = 0,
	   CREATE_T,						//task
	   YIELD,
	   TERMINATE,
	   SUSPEND,
	   RESUME,
	   SLEEP,
	   
	   CREATE_E,						//event
	   WAIT_E,
	   SIGNAL_E,
	   
	   CREATE_EG,						//event groups
	   DESTROY_EG,
	   SET_EG_BITS,
	   CLEAR_EG_BITS,
	   WAIT_EG,
	   GET_EG_BITS,
	   
	   CREATE_M,						//mutex
	   DESTROY_M,
	   LOCK_M,
	   UNLOCK_M,
	   
	   CREATE_SEM,						//semaphores
	   DESTROY_SEM,
	   GIVE_SEM,
	   GET_SEM  
   } KERNEL_REQUEST;



   /*Process descriptor for a task*/
   typedef struct ProcessDescriptor
   {
	   PID pid;										//An unique process ID for this task.
	   PRIORITY pri;								//The priority of this task, from 0 (highest) to 10 (lowest).
	   PROCESS_STATE state;							//What's the current state of this task?
	   unsigned char workSpace[WORKSPACE];			//Data memory allocated to this process.
	   unsigned char *sp;							//stack pointer into the "workSpace".
	   taskfuncptr code;							//The function to be executed when this process is running.
	   int arg;										//User specified arg for the task 
	   
	   /*Used to pass requests from task to kernel*/
	   KERNEL_REQUEST request;						//What the task want the kernel to do (when needed).
	   int request_args[MAX_KERNEL_ARGS];			//What values are needed for the specified kernel request.
	   int request_ret;								//Value returned by the kernel after handling the request
	   TICK request_timeout;						
	   
	   /*Used for task suspension/resuming*/
	   PROCESS_STATE last_state;					//What's the PREVIOUS state of this task? Used for task suspension/resume.
	   
	   /*Used for other scheduling modes*/
	   #ifdef PREVENT_STARVATION
	   unsigned int starvation_ticks;
	   #endif
	   
   } PD;



/************************************************************************/
/*					SHARED KERNEL VARIABLES                             */
/************************************************************************/

//These kernel variables are accessible by other kernel modules and the OS internally
extern volatile PtrList ProcessList;
extern volatile PD* Current_Process;	
extern volatile unsigned int KernelActive;
extern volatile unsigned int Kernel_Request_Cswitch;	
extern volatile ERROR_CODE err;



/************************************************************************/
/*					SHARED KERNEL FUNCTIONS                             */
/************************************************************************/

PD* findProcessByPID(int pid);


#endif /* KERNEL_INTERNAL_H_ */