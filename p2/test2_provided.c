#include "rtos/os.h"
#include "rtos/kernel/kernel.h"
#include <stdio.h>

#include "rtos/kernel/others/pid_queue.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED_PIN_MASK 0x80			//Pin 13 = PB7
EVENT e1;
MUTEX m1;
EVENT evt;
MUTEX mut;

void Idle()
{
	for(;;);
};

/************************************************************************/
/*				Test 0: Task Suspension, Resume, Sleep, Yield		    */
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
/*							Test 1: Events			                    */
/************************************************************************/


void event_wait_test()
{
	//Test normal signaling
	e1 = Event_Init();
	printf("Waiting for event e1...\n");
	Event_Wait(e1);
	printf("Signal for event e1 received!\n");
	
	e1 = Event_Init();
	Task_Sleep(100);
	
	//Test pre signaling
	printf("Waiting for event e1...\n");
	Event_Wait(e1);
	printf("Signal for event e1 received!\n");
	Task_Yield();
}

void event_signal_test()
{
	printf("Signaling event e1...\n");
	Event_Signal(e1);
	Task_Yield();
	
	printf("Signaling event e1 twice...\n");
	Event_Signal(e1);
	Event_Signal(e1);
	Task_Yield();
}

void test1()
{
	Task_Create(event_wait_test, 4, 0);
	Task_Create(event_signal_test, 5, 0);
}


/************************************************************************/
/*						Test 2: Priority			                    */
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
		printf("GETTING 2...\n");
		Semaphore_Get(s1, 2);
		printf("GOT 2\n");
		Task_Sleep(100);
	}
}

void test3()
{
	s1 = Semaphore_Init(0, 0);
	Task_Create(sem_task0, 4, 0);
	Task_Create(sem_task1, 5, 0);
	//Task_Create(sem_task1, 5, 0);
	//Task_Create(sem_task1, 5, 0);
	//Task_Create(sem_task1, 5, 0);
	//Task_Create(sem_task1, 5, 0);
	//Task_Create(sem_task1, 5, 0);
}


/************************************************************************/
/*								Test 4			                        */
/************************************************************************/


MUTEX m1;

void mut_t1()
{
	printf("T1 locking m1...\n");
	Mutex_Lock(m1);
	printf("T1 locked m1!\n");
	Task_Sleep(250);
	printf("T1 unlocking m1...\n");
	Mutex_Unlock(m1);
	printf("T1 unlocked m1!\n");
	//Task_Yield();
    for(;;);
}

void mut_t2()
{
	Task_Sleep(50);
	printf("T2 locking m1...\n");
	Mutex_Lock(m1);
	printf("T2 locked m1!\n");
	Task_Sleep(250);
	printf("T2 unlocking m1...\n");
	Mutex_Unlock(m1);
	printf("T2 unlocked m1!\n");
	
    for(;;);
}


void test4()
{
    m1 = Mutex_Init();

    Task_Create(mut_t1, 1, 0);
    Task_Create(mut_t2, 2, 0);

    //Task_Terminate();
	//for(;;);
}


/************************************************************************/
/*					Test 5: Preemptive Schedulling		                */
/************************************************************************/


void ps1()
{
	for(;;)
	{
		puts("A");
		//Task_Yield();
	}

}

void ps2()
{
	for(;;)
	{
		puts("B");
		//Task_Yield();
	}
}

void ps3()
{
	for(;;)
	{
		puts("C");
		//Task_Yield();
	}
}

void ps4()
{
	for(;;)
	{
		puts("D");
		//Task_Yield();
	}
}

void test5()
{
	//These tasks tests priority scheduling
	Task_Create(ps1, 1, 0);
	Task_Create(ps2, 1, 0);
	Task_Create(ps3, 1, 0);
	Task_Create(ps4, 1, 0);
}


/************************************************************************/
/*				Test 6: Mutex with Priority Inheritance	                */
/************************************************************************/
/*
 * priority q > r > p
 * 
 * expected order
 * p q p q r p
 *
 * q                 create(r)  lock(attempt)					   lock(switch in)   terminate   
 * r																						  runs  terminate
 * p  lock creates(q)                         (gain priority)unlock                                           terminate
 */

MUTEX mut;

void task_r()
{
	printf("r: terminating\n");
	Task_Terminate();
}

void task_q()
{
	printf("q: hello, gonna create R\n");

	Task_Create(task_r, 2, 0);
	printf("q: gonna try to lock mut\n");
	Mutex_Lock(mut);
	printf("q: I got into the mutex yeah! But I will let it go\n");
	Mutex_Lock(mut);
	printf("q: I am gonna die, good bye world\n");
	Task_Terminate();
}

void task_p()
{
	printf("p:hello, gonna lock mut\n");
	Mutex_Lock(mut);
	printf("p: gonna create q\n");
	Task_Create(task_q, 1, 0);
	Task_Yield();
	printf("p: gonna unlock mut\n");
	Mutex_Unlock(mut);
	Task_Yield();
	Task_Terminate();
}

void test6()
{
	mut = Mutex_Init();
	Task_Create(task_p, 3, 0);
}


/************************************************************************/
/*					Test 7: Starvation Prevention		                */
/************************************************************************/

void test7()
{
	//These tasks tests priority scheduling
	Task_Create(ps1, 1, 0);
	Task_Create(ps2, 2, 0);
	Task_Create(ps3, 3, 0);
	Task_Create(ps4, 4, 0);
}



/************************************************************************/
/*						Test 8: Event Groups				            */
/************************************************************************/

EVENT_GROUP eg1;

void egt1()
{
	printf("T1 waking up event 0...\n");
	Event_Group_Set_Bits(eg1, (1<<0));
	
	//printf("T1 sleeping...");
	Task_Sleep(100);
	//print_processes();
	printf("T1 waking up event 6...\n");
	Event_Group_Set_Bits(eg1, (1<<6));
	//print_processes();
	Task_Sleep(100);
	
	printf("T1 waking up event 9...\n");
	Event_Group_Set_Bits(eg1, (1<<9));
	//print_processes();
	Task_Sleep(100);
	
	printf("T1 waking up event 3...\n");
	Event_Group_Set_Bits(eg1, (1<<3));
	//print_processes();
	
	Task_Terminate();
	
	//for(;;)	Task_Yield();
}

void egt2()
{
	printf("T2 waiting for event 3, 6...\n");
	Event_Group_Wait_Bits(eg1, (1<<3) | (1<<6), 1, 0);
	printf("T2 got bit 3 AND 6!\n");
	
	//Task_Terminate();
	
	for(;;)	Task_Yield();
}

void egt3()
{
	printf("T3 waiting for event 6, 9...\n");
	Event_Group_Wait_Bits(eg1, (1<<6) | (1<<9), 0, 0);
	printf("T3 got bit 6 OR 9!\n");
	
	//Task_Terminate();
	
	for(;;)	Task_Yield();
}

void egt4()
{
	printf("T4 waiting for event 10...\n");
	Event_Group_Wait_Bits(eg1, (1<<10), 0, 500);
	printf("T4 Timed out!\n");
	
	//Task_Terminate();
	
	for(;;)	Task_Yield();

}

void test8()
{
	eg1 = Event_Group_Init();
	
	Task_Create(egt1, 1, 0);
	Task_Create(egt2, 1, 0);
	Task_Create(egt3, 1, 0);
	Task_Create(egt4, 1, 0);
	
	//Task_Terminate();
}


/************************************************************************/
/*						Entry point for application		                */
/************************************************************************/

void a_main()
{
	int test_set = 8;				//Which set of tests to run?

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
	else if(test_set == 4)
	test4();
	else if(test_set == 5)
	test5();
	else if(test_set == 6)
	test6();
	else if(test_set == 7)
	test7();
	else if(test_set == 8)
	test8();

	else
	{
		printf("Invalid testing set specified...\n");
		while(1);
	}
	
	
	OS_Start();
}
