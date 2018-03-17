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
PID Task_Create(taskfuncptr f, size_t stack_size, PRIORITY py, int arg)
{
   PID retval;
   
   //Run the task creation through kernel if it's running already
   if (KernelActive) 
   {
     Disable_Interrupt();
	 Current_Process->request_args[0].ptr = f;
	 Current_Process->request_args[1].val = stack_size;
	 Current_Process->request_args[2].val = py;
	 Current_Process->request_args[3].val = arg;
	 Current_Process->request = TASK_CREATE;
     Enter_Kernel();									//Interrupts are automatically reenabled once the kernel is exited
	 
	 //Retrieve the return value once the kernel exits
	 retval = Current_Process->request_retval;
   } 
   else 
	  retval = Kernel_Create_Task_Direct(f,stack_size,py,arg);				//If kernel hasn't started yet, manually create the task
   
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
	Current_Process -> request = TASK_TERMINATE;
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
    Current_Process->request = TASK_YIELD;
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
void Task_Suspend(PID p)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = TASK_SUSPEND;
	Current_Process->request_args[0].val = p;
	Enter_Kernel();
}

void Task_Resume(PID p)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = TASK_RESUME;
	Current_Process->request_args[0].val = p;
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
	Current_Process->request = TASK_SLEEP;
	Current_Process->request_timeout = t;
	Enter_Kernel();
}


/************************************************************************/
/*						Events related API			                    */
/************************************************************************/
#ifdef EVENT_ENABLED

EVENT Event_Create(void)
{
	EVENT retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = E_CREATE;
		Enter_Kernel();
		
		retval = Current_Process->request_retval;
	}
	else
		retval = Kernel_Create_Event();		//Call the kernel function directly if kernel has not started yet.	
	
	//Return zero as Event ID if the event creation process gave errors. Note that the smallest valid event ID is 1
	if (err == MAX_OBJECT_ERR)
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
	Current_Process->request = E_WAIT;
	Current_Process->request_args[0].val = e;
	Enter_Kernel();
	
}

void Event_Signal(EVENT e)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = E_SIGNAL;
	Current_Process->request_args[0].val = e;
	Enter_Kernel();	
}

#endif

/************************************************************************/
/*						Mutex related API			                    */
/************************************************************************/

#ifdef MUTEX_ENABLED

MUTEX Mutex_Create(void)
{
	MUTEX retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = MUT_CREATE;
		Enter_Kernel();
		
		retval = Current_Process->request_retval;
	}
	else
		retval = Kernel_Create_Mutex();	//Call the kernel function directly if OS hasn't start yet
		
	//Return zero as Mutex ID if the mutex creation process gave errors. Note that the smallest valid mutex ID is 1
	if (err == MAX_OBJECT_ERR)
	return 0;
	
	#ifdef DEBUG
	printf("Created Mutex: %d\n", Last_MutexID);
	#endif
	
	return retval;
}

int Mutex_Destroy(MUTEX m)
{
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = MUT_DESTROY;
		Current_Process->request_args[0].val = m;
		Enter_Kernel();
	}
	
	return (err > 0)? 0:1;	//return 1 if no error, return 0 if semaphore was not found
}

void Mutex_Lock(MUTEX m)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = MUT_LOCK;
	Current_Process->request_args[0].val = m;
	Enter_Kernel();
}

void Mutex_Unlock(MUTEX m)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = MUT_UNLOCK;
	Current_Process->request_args[0].val = m;
	Enter_Kernel();
}
#endif

/************************************************************************/
/*						Semaphore related API			                */
/************************************************************************/
#ifdef SEMAPHORE_ENABLED

SEMAPHORE Semaphore_Create(int initial_count, unsigned int is_binary)
{
	SEMAPHORE retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = SEM_CREATE;
		Current_Process->request_args[0].val = initial_count;
		Current_Process->request_args[1].val = is_binary;
		Enter_Kernel();
		
		retval = Current_Process->request_retval;
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

int Semaphore_Destroy(SEMAPHORE s)
{
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = SEM_DESTROY;
		Current_Process->request_args[0].val = s;
		Enter_Kernel();
	}
	
	return (err > 0)? 0:1;	//return 1 if no error, return 0 if semaphore was not found
}

void Semaphore_Give(SEMAPHORE s, unsigned int amount)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = SEM_GIVE;
	Current_Process->request_args[0].val = s;
	Current_Process->request_args[1].val = amount;
	Enter_Kernel();
}

void Semaphore_Get(SEMAPHORE s, unsigned int amount)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = SEM_GET;
	Current_Process->request_args[0].val = s;
	Current_Process->request_args[1].val = amount;
	Enter_Kernel();
}

#endif

/************************************************************************/
/*						Event Group related API			                */
/************************************************************************/
#ifdef EVENT_GROUP_ENABLED

EVENT_GROUP Event_Group_Create(void)
{
	EVENT_GROUP retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = EG_CREATE;
		Enter_Kernel();
		
		retval = Current_Process->request_retval;
	}
	else
		retval = Kernel_Create_Event_Group();	//Call the kernel function directly if kernel has not started yet.
	
	
	//Return zero as Event ID if the event creation process gave errors. Note that the smallest valid event ID is 1
	if (err == MAX_OBJECT_ERR)
	return 0;
	
	#ifdef DEBUG
	printf("Created Event Group: %d\n", Last_Event_Group_ID);
	#endif
	
	return retval;
}

int Event_Group_Destroy(EVENT_GROUP eg)
{
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = EG_DESTROY;
		Current_Process->request_args[0].val = eg;
		Enter_Kernel();
	}
	
	return (err > 0)? 0:1;	//return 1 if no error, return 0 if semaphore was not found
}

void Event_Group_Set_Bits(EVENT_GROUP e, unsigned int bits_to_set)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = EG_SETBITS;
	Current_Process->request_args[0].val = e;
	Current_Process->request_args[1].val = bits_to_set;
	Enter_Kernel();
}

void Event_Group_Clear_Bits(EVENT_GROUP e, unsigned int bits_to_clear)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = EG_CLEARBITS;
	Current_Process->request_args[0].val = e;
	Current_Process->request_args[1].val = bits_to_clear;
	Enter_Kernel();
}

void Event_Group_Wait_Bits(EVENT_GROUP e, unsigned int bits_to_wait, unsigned int wait_all_bits, TICK timeout)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	
	Disable_Interrupt();
	Current_Process->request = EG_WAITBITS;
	Current_Process->request_args[0].val = e;
	Current_Process->request_args[1].val = bits_to_wait;
	Current_Process->request_args[2].val = wait_all_bits;
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
	Current_Process->request = EG_GETBITS;
	Enter_Kernel();
	
	return Current_Process->request_retval;
}

#endif


/************************************************************************/
/*						Mailbox related API			                */
/************************************************************************/
#ifdef MAILBOX_ENABLED

MAILBOX Mailbox_Create(unsigned int capacity)
{
	MAILBOX retval;
	
	if(KernelActive)
	{
		Disable_Interrupt();
		Current_Process->request = MB_CREATE;
		Current_Process->request_args[0].val = capacity;
		Enter_Kernel();
		
		retval = Current_Process->request_retval;
	}
	else
		retval = Kernel_Create_Mailbox_Direct(capacity);		//Call the kernel function directly if OS hasn't start yet
	
	if(err != NO_ERR)
		return 0;
	
	#ifdef DEBUG
	printf("Created Mailbox: %d\n", Last_MailboxID);
	#endif
	
	return retval;
}


void Mailbox_Destroy(MAILBOX mb)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
		
	Disable_Interrupt();
	Current_Process->request = MB_DESTROY;
	Current_Process->request_args[0].val = mb;
	Enter_Kernel();
}


int Mailbox_Destroy_Mail(MAIL* received)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return 0;
	}
	
	Disable_Interrupt();
	Current_Process->request = MB_DESTROYM;
	Current_Process->request_args[0].ptr = received;
	Enter_Kernel();
	
	return Current_Process->request_retval;
}


int Mailbox_Check_Mail(MAILBOX mb)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return 0;
	}
	
	Disable_Interrupt();
	Current_Process->request = MB_CHECKMAIL;
	Current_Process->request_args[0].val = mb;
	Enter_Kernel();
	
	return Current_Process->request_retval;
}


int Mailbox_Send_Mail(MAILBOX mb, void *msg, size_t msg_size)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return 0;
	}
	
	Disable_Interrupt();
	Current_Process->request = MB_SENDMAIL;
	Current_Process->request_args[0].val = mb;
	Current_Process->request_args[1].ptr = msg;
	Current_Process->request_args[2].val = msg_size;
	Enter_Kernel();
	
	return Current_Process->request_retval;
}


int Mailbox_Get_Mail(MAILBOX mb, MAIL* received)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return 0;
	}
		
	Disable_Interrupt();
	Current_Process->request = MB_GETMAIL;
	Current_Process->request_args[0].val = mb;
	Current_Process->request_args[1].ptr = received;
	Enter_Kernel();
	
	return Current_Process->request_retval;
}



#endif