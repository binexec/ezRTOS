#include "rtos/os.h"
#include "rtos/kernel/kernel.h"
#include <stdio.h>

#include "rtos/kernel/others/queue.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED_PIN_MASK 0x80			//Pin 13 = PB7
EVENT e1;
MUTEX m1;
EVENT evt;
MUTEX mut;

/************************************************************************/
/*								Test 0			                        */
/************************************************************************/

void Ping()
{
	for(;;)
	{
		PORTB |= LED_PIN_MASK;		//Turn on onboard LED
		printf("A!\n");
		Task_Sleep(50);
		//Task_Terminate();
		Task_Yield();
	}
}

void Pong()
{
	for(;;)
	{
		PORTB &= ~LED_PIN_MASK;		//Turn off onboard LED
		printf("B!\n");
		Task_Sleep(50);
		Task_Yield();
	}
}

void Peng()
{
	for(;;)
	{
		printf("C!\n");
		Task_Sleep(50);
		Task_Yield();
	}
}

void suspend_pong()
{
	for(;;)
	{
		Task_Sleep(300);
		printf("SUSPENDING PONG!\n");
		Task_Suspend(Pong);
		Task_Yield();
		
		Task_Sleep(300);
		printf("RESUMING PONG!\n");
		Task_Resume(Pong);
		Task_Yield();
	}
	
}

void test0()
{
	DDRB = LED_PIN_MASK;			//Set pin 13 as output
	Task_Create(Ping, 6, 210);
	Task_Create(Pong, 6, 205);
	Task_Create(suspend_pong, 4, 0);
	Task_Create(Peng, 6, 205);
}


/************************************************************************/
/*								Test 1			                        */
/************************************************************************/


void event_wait_test()
{

	//Test normal signaling
	e1 = Event_Init();
	printf("Waiting for event %d...\n", e1);
	Event_Wait(e1);
	printf("Signal for event %d received! Total events: %d\n\n\n", e1, getEventCount(e1));
	e1 = Event_Init();
	Task_Sleep(100);
	//Task_Yield();
	
	//Test pre signaling
	printf("Waiting for event %d...\n", e1);
	Event_Wait(e1);
	printf("Signal for event %d received! Total events: %d\n", e1, getEventCount(e1));
	Task_Yield();
}

void event_signal_test()
{
	printf("Signalling event %d...\n", e1);
	Event_Signal(e1);
	printf("SIGNALED!\n");
	Task_Yield();
	
	printf("Signalling event %d...\n", e1);
	Event_Signal(e1);
	Event_Signal(e1);
	printf("SIGNALED!\n");
	Task_Yield();
}

void test1()
{
	//These tasks tests events
	Task_Create(event_wait_test, 4, 0);
	Task_Create(event_signal_test, 5, 0);
}


/************************************************************************/
/*								Test 2			                        */
/************************************************************************/


void priority1()
{
	for(;;)
	{
		printf("Hello from 1!\n");
		Task_Sleep(300);
	}
}

void priority2()
{
	for(;;)
	{
		printf("Hello from 2!\n");
		Task_Sleep(200);
	}
}

void priority3()
{
	for(;;)
	{
		printf("Hello from 3!\n");
		Task_Sleep(100);
	}
}

void test2()
{
	//These tasks tests priority scheduling
	Task_Create(priority1, 1, 0);
	Task_Create(priority2, 2, 0);
	Task_Create(priority3, 3, 0);
}


/************************************************************************/
/*								Test 3			                        */
/************************************************************************/

SEMAPHORE s1;

void sem_task0()		//producer
{
	while(1)
	{
		Semaphore_Give(s1, 5);
		printf("***GAVE 5\n");
		Task_Sleep(1000);
	}
}

void sem_task1()		//consumer
{
	while(1)
	{
		printf("GETTING 1...\n");
		Semaphore_Get(s1, 2);
		printf("GOT 1\n");
		Task_Sleep(100);
	}
}

void test3()
{
	s1 = Semaphore_Init(0, 0);
	Task_Create(sem_task0, 4, 0);
	Task_Create(sem_task1, 5, 0);
	Task_Create(sem_task1, 5, 0);
	Task_Create(sem_task1, 5, 0);
	Task_Create(sem_task1, 5, 0);
	Task_Create(sem_task1, 5, 0);
	Task_Create(sem_task1, 5, 0);
}



/************************************************************************/
/*						Entry point for application		                */
/************************************************************************/

void a_main()
{
	int test_set = 3;				//Which set of tests to run?

	OS_Init();
	
	//These tasks tests ctxswitching, suspension, resume, and sleep
	if(test_set == 0)
		test0();
	else if(test_set == 1)
		test1();	
	else if(test_set == 2)
		test2();
	else if(test_set == 3)
		test3();
	
	else
	{
		printf("Invalid testing set specified...\n");
		while(1);
	}
	
	
	OS_Start();
}
