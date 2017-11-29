#include "os.h"
#include "kernel.h"

#define LED_PIN_MASK 0x80			//Pin 13 = PB7
EVENT e1;
MUTEX m1;
EVENT evt;
MUTEX mut;

/************************************************************************/
/*							CODE FOR TESTING                            */
/************************************************************************/

void Ping()
{
	for(;;)
	{
		PORTB |= LED_PIN_MASK;		//Turn on onboard LED
		printf("PING!\n");
		Task_Sleep(50);
		Task_Yield();
	}
}

void Pong()
{
	for(;;)
	{
		PORTB &= ~LED_PIN_MASK;		//Turn off onboard LED
		printf("PONG!\n");
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
		Task_Suspend(findPIDByFuncPtr(Pong));
		Task_Yield();
		
		Task_Sleep(300);
		printf("RESUMING PONG!\n");
		Task_Resume(findPIDByFuncPtr(Pong));
		Task_Yield();
	}
	
}


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

/*Entry point for application*/
/*
void a_main()
{
	int test_set = 0;				//Which set of tests to run?

	OS_Init();
	
	//These tasks tests ctxswitching, suspension, resume, and sleep
	if(test_set == 0)
	{
		DDRB = LED_PIN_MASK;			//Set pin 13 as output
		Task_Create(Ping, 6, 210);
		Task_Create(Pong, 6, 205);
		Task_Create(suspend_pong, 4, 0);
	}
	else if(test_set == 1)
	{
		//These tasks tests events
		Task_Create(event_wait_test, 4, 0);
		Task_Create(event_signal_test, 5, 0);
	}
	else if(test_set == 2)
	{
		//These tasks tests priority scheduling
		Task_Create(priority1, 1, 0);
		Task_Create(priority2, 2, 0);
		Task_Create(priority3, 3, 0);
	}
	OS_Start();
}
*/