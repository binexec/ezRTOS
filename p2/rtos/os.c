#include "os.h"
#include "kernel.h"
#include "hardware/uart/uart.h"

#include <stdio.h>

#define DEBUG
extern void a_main();					//External entry point for application once kernel and OS has initialized.


/************************************************************************/
/*						   RTOS API FUNCTIONS                           */
/************************************************************************/

void OS_Init(void)	{Kernel_Reset();}
void OS_Start(void)	{Kernel_Start();}


/* OS call to create a new task */
PID Task_Create(voidfuncptr f, PRIORITY py, int arg)
{
   //Run the task creation through kernel if it's running already
   if (KernelActive) 
   {
     Enter_Critical_Section();
	 
	 //Fill in the parameters for the new task into CP
	 Current_Process->pri = py;
	 Current_Process->arg = arg;
     Current_Process->request = CREATE_T;
     Current_Process->code = f;

     Enter_Kernel();
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
	
	PORTB |= (1<<PB2);

	Enter_Critical_Section();
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

    Enter_Critical_Section();
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

//NOTE: Calling task should not try and suspend itself!
void Task_Suspend(PID p)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Enter_Critical_Section();
	
	//Sets up the kernel request fields in the PD for this task
	Current_Process->request = SUSPEND;
	Current_Process->request_arg = p;
	//printf("SUSPENDING: %u\n", Cp->request_arg);
	Enter_Kernel();
}

void Task_Resume(PID p)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Enter_Critical_Section();
	
	//Sets up the kernel request fields in the PD for this task
	Current_Process->request = RESUME;
	Current_Process->request_arg = p;
	//printf("RESUMING: %u\n", Cp->request_arg);
	Enter_Kernel();
}

/*Puts the calling task to sleep for AT LEAST t ticks.*/
void Task_Sleep(TICK t)
{
	
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Enter_Critical_Section();
	
	Current_Process->request = SLEEP;
	Current_Process->request_arg = t;

	Enter_Kernel();
}

/*Initialize an event object*/
EVENT Event_Init(void)
{
	if(KernelActive)
	{
		Enter_Critical_Section();
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
	
	Enter_Critical_Section();
	
	Current_Process->request = WAIT_E;
	Current_Process->request_arg = e;
	Enter_Kernel();
	
}

void Event_Signal(EVENT e)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Enter_Critical_Section();
	
	Current_Process->request = SIGNAL_E;
	Current_Process->request_arg = e;
	Enter_Kernel();	
}

MUTEX Mutex_Init(void)
{
	
	
	if(KernelActive)
	{
		Enter_Critical_Section();
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
	
	
	Enter_Critical_Section();
	
	Current_Process->request = LOCK_M;
	Current_Process->request_arg = m;
	Enter_Kernel();
}

void Mutex_Unlock(MUTEX m)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Enter_Critical_Section();
	
	Current_Process->request = UNLOCK_M;
	Current_Process->request_arg = m;
	Enter_Kernel();
}

/*Don't use main function for application code. Any mandatory kernel initialization should be done here*/
int main() 
{
   //Enable STDIN/OUT to UART redirection for debugging
   #ifdef DEBUG
	uart_init();
	uart_setredir();
	printf("STDOUT->UART!\n");
   #endif  
   
   a_main();
   
}

