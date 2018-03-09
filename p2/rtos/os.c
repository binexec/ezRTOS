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
PID Task_Create(taskfuncptr f, PRIORITY py, int arg)
{
   PID retval;
   
   //Run the task creation through kernel if it's running already
   if (KernelActive) 
   {
     Disable_Interrupt();
	 Current_Process->request_args[0] = f;				//Will throw warning about converting from pointer to int; just ignore it. Ensure pointers are 32-bits or less for target arch 
	 Current_Process->request_args[1] = py;
	 Current_Process->request_args[2] = arg;
	 Current_Process->request = CREATE_T;
     Enter_Kernel();									//Interrupts are automatically reenabled once the kernel is exited
	 
	 //Retrieve the return value once the kernel exits
	 retval = Current_Process->request_ret;
   } 
   else 
	  retval = Kernel_Create_Task_Direct(f,py,arg);				//If kernel hasn't started yet, manually create the task
   
   //Return zero as PID if the task creation process gave errors. Note that the smallest valid PID is 1
   if (err == MAX_PROCESS_ERR)
		return 0;
   
   #ifdef DEBUG
	printf("Created PID: %d\n", retval);
   #endif
   
   return retval;
}

/* The calling task terminates itself. */
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
void Task_Suspend(taskfuncptr f)
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

void Task_Resume(taskfuncptr f)
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
	Current_Process->request_timeout = t;
	Enter_Kernel();
}


/************************************************************************/
/*						Events related API			                    */
/************************************************************************/
#ifdef EVENT_ENABLED

EVENT Event_Init(void)
{
	EVENT retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = CREATE_E;
		Enter_Kernel();
		
		retval = Current_Process->request_ret;
	}
	else
		retval = Kernel_Create_Event();		//Call the kernel function directly if kernel has not started yet.	
	
	//Return zero as Event ID if the event creation process gave errors. Note that the smallest valid event ID is 1
	if (err == MAX_EVENT_ERR)
		return 0;
	
	#ifdef DEBUG
	printf("Created Event: %d\n", Last_EventID);
	#endif
	
	return retval;
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

#endif

/************************************************************************/
/*						Mutex related API			                    */
/************************************************************************/

#ifdef MUTEX_ENABLED

MUTEX Mutex_Init(void)
{
	MUTEX retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = CREATE_M;
		Enter_Kernel();
		
		retval = Current_Process->request_ret;
	}
	else
		retval = Kernel_Create_Mutex();	//Call the kernel function directly if OS hasn't start yet
		
	//Return zero as Mutex ID if the mutex creation process gave errors. Note that the smallest valid mutex ID is 1
	if (err == MAX_MUTEX_ERR)
	return 0;
	
	#ifdef DEBUG
	printf("Created Mutex: %d\n", Last_MutexID);
	#endif
	
	return retval;
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
#endif

/************************************************************************/
/*						Semaphore related API			                */
/************************************************************************/
#ifdef SEMAPHORE_ENABLED

SEMAPHORE Semaphore_Init(int initial_count, unsigned int is_binary)
{
	SEMAPHORE retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = CREATE_SEM;
		Current_Process->request_args[0] = initial_count;
		Current_Process->request_args[1] = is_binary;
		Enter_Kernel();
		
		retval = Current_Process->request_ret;
	}
	else
		retval = Kernel_Create_Semaphore_Direct(initial_count, is_binary);		//Call the kernel function directly if OS hasn't start yet
		
	//Return the created semaphore's ID, or 0 if failed
	if(err != NO_ERR)
		return 0;
	
	#ifdef DEBUG
	printf("Created Semaphore: %d\n", Last_SemaphoreID);
	#endif
	
	return retval;
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

#endif

/************************************************************************/
/*						Event Group related API			                */
/************************************************************************/
#ifdef EVENT_GROUP_ENABLED

EVENT_GROUP Event_Group_Init()
{
	EVENT_GROUP retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = CREATE_EG;
		Enter_Kernel();
		
		retval = Current_Process->request_ret;
	}
	else
		retval = Kernel_Create_Event_Group();	//Call the kernel function directly if kernel has not started yet.
	
	
	//Return zero as Event ID if the event creation process gave errors. Note that the smallest valid event ID is 1
	if (err == MAX_EVENT_ERR)
	return 0;
	
	#ifdef DEBUG
	printf("Created Event Group: %d\n", Last_Event_Group_ID);
	#endif
	
	return retval;
}

void Event_Group_Set_Bits(EVENT_GROUP e, unsigned int bits_to_set)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = SET_EG_BITS;
	Current_Process->request_args[0] = e;
	Current_Process->request_args[1] = bits_to_set;
	Enter_Kernel();
}

void Event_Group_Clear_Bits(EVENT_GROUP e, unsigned int bits_to_clear)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = CLEAR_EG_BITS;
	Current_Process->request_args[0] = e;
	Current_Process->request_args[1] = bits_to_clear;
	Enter_Kernel();
}

void Event_Group_Wait_Bits(EVENT_GROUP e, unsigned int bits_to_wait, unsigned int wait_all_bits, TICK timeout)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = WAIT_EG;
	Current_Process->request_args[0] = e;
	Current_Process->request_args[1] = bits_to_wait;
	Current_Process->request_args[2] = wait_all_bits;
	Current_Process->request_timeout = timeout;
	Enter_Kernel();
}


unsigned int Event_Group_Get_Bits(EVENT_GROUP e)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return 0;
	}
	
	Disable_Interrupt();
	Current_Process->request = GET_EG_BITS;
	Enter_Kernel();
	
	return Current_Process->request_ret;
}

#endif