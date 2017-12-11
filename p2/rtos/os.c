#include "os.h"
#include "kernel/kernel.h"

#include <stdio.h>

#define DEBUG

extern void a_main();					//External entry point for application once kernel and OS has initialized.


/************************************************************************/
/*					RTOS Initialization Functions                       */
/************************************************************************/

void OS_Init(void)	
{
	stdio_init();
	Kernel_Reset();
}

void OS_Start(void)	
{
	Kernel_Start();
}

/*Don't use main function for application code. Any mandatory kernel initialization should be done here*/
int main()
{
	a_main();		//Call the user's application entry point instead
}


/************************************************************************/
/*						Task/Thread related API                         */
/************************************************************************/


/* OS call to create a new task */
PID Task_Create(voidfuncptr f, PRIORITY py, int arg)
{
   //Run the task creation through kernel if it's running already
   if (KernelActive) 
   {
     Disable_Interrupt();
	 
	 //Fill in the parameters for the new task into CP
	 Current_Process->pri = py;
	 Current_Process->arg = arg;
     Current_Process->request = CREATE_T;
     Current_Process->code = f;
	 
     Enter_Kernel();		//Interrupts are automatically reenabled once the kernel is exited
   } 
   else 
	   Kernel_Create_Task(f,py,arg);		//If kernel hasn't started yet, manually create the task
   
   //Return zero as PID if the task creation process gave errors. Note that the smallest valid PID is 1
   if (err == MAX_PROCESS_ERR)
		return 0;
   
   #ifdef DEBUG
	printf("Created PID: %d\n", Last_PID);
   #endif
   
   return Last_PID;
}

/* The calling task terminates itself. */
/*TODO: CLEAN UP EVENTS AND MUTEXES*/
void Task_Terminate()
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}

	Disable_Interrupt();
	Current_Process -> request = TERMINATE;
	Enter_Kernel();			
}

/* The calling task gives up its share of the processor voluntarily. Previously Task_Next() */
void Task_Yield() 
{	
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}

    Disable_Interrupt();
    Current_Process ->request = YIELD;
    Enter_Kernel();
}

int Task_GetArg()
{
	if (KernelActive) 
		return Current_Process->arg;
	else
		return -1;
}

//Calling task should not try and suspend itself?
void Task_Suspend(voidfuncptr f)
{
	
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = SUSPEND;
	Current_Process->request_args[0] = findPIDByFuncPtr(f);
	Enter_Kernel();
}

void Task_Resume(voidfuncptr f)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = RESUME;
	Current_Process->request_args[0] = findPIDByFuncPtr(f);
	Enter_Kernel();
}

/*Puts the calling task to sleep for AT LEAST t ticks.*/
void Task_Sleep(TICK t)
{
	
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = SLEEP;
	Current_Process->request_args[0] = t;
	Enter_Kernel();
}


/************************************************************************/
/*						Events related API			                    */
/************************************************************************/


/*Initialize an event object*/
EVENT Event_Init(void)
{
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = CREATE_E;
		Enter_Kernel();
	}
	else
		Kernel_Create_Event();	//Call the kernel function directly if kernel has not started yet.
	
	
	//Return zero as Event ID if the event creation process gave errors. Note that the smallest valid event ID is 1
	if (err == MAX_EVENT_ERR)
		return 0;
	
	#ifdef DEBUG
	printf("Created Event: %d\n", Last_EventID);
	#endif
	
	return Last_EventID;
}

void Event_Wait(EVENT e)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = WAIT_E;
	Current_Process->request_args[0] = e;
	Enter_Kernel();
	
}

void Event_Signal(EVENT e)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = SIGNAL_E;
	Current_Process->request_args[0] = e;
	Enter_Kernel();	
}


/************************************************************************/
/*						Mutex related API			                    */
/************************************************************************/


MUTEX Mutex_Init(void)
{
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = CREATE_M;
		Enter_Kernel();
	}
	else
		Kernel_Create_Mutex();	//Call the kernel function directly if OS hasn't start yet
	
	
	//Return zero as Mutex ID if the mutex creation process gave errors. Note that the smallest valid mutex ID is 1
	if (err == MAX_MUTEX_ERR)
	return 0;
	
	#ifdef DEBUG
	printf("Created Mutex: %d\n", Last_MutexID);
	#endif
	
	return Last_MutexID;
}

void Mutex_Lock(MUTEX m)
{
	
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = LOCK_M;
	Current_Process->request_args[0] = m;
	Enter_Kernel();
}

void Mutex_Unlock(MUTEX m)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = UNLOCK_M;
	Current_Process->request_args[0] = m;
	Enter_Kernel();
}

/************************************************************************/
/*						Semaphore related API			                */
/************************************************************************/

SEMAPHORE Semaphore_Init(int initial_count, unsigned int is_binary)
{
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = CREATE_SEM;
		Current_Process->request_args[0] = initial_count;
		Current_Process->request_args[1] = is_binary;
		Enter_Kernel();
	}
	else
		Kernel_Create_Semaphore(initial_count, is_binary);	//Call the kernel function directly if OS hasn't start yet
		
	//Return the created semaphore's ID, or 0 if failed
	if(err != NO_ERR)
		return 0;
	
	#ifdef DEBUG
	printf("Created Semaphore: %d\n", Last_SemaphoreID);
	#endif
	
	return Last_SemaphoreID;
	
	
}

void Semaphore_Give(SEMAPHORE s, unsigned int amount)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = GIVE_SEM;
	Current_Process->request_args[0] = s;
	Current_Process->request_args[1] = amount;
	Enter_Kernel();
	
}

void Semaphore_Get(SEMAPHORE s, unsigned int amount)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = GET_SEM;
	Current_Process->request_args[0] = s;
	Current_Process->request_args[1] = amount;
	Enter_Kernel();
}