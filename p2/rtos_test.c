#include "rtos/os.h"
#include "rtos/kernel/kernel.h"
#include <stdio.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED_PIN_MASK 0x80			//Pin 13 = PB7

#define TASK_STACK_SIZE					256				//Stack size (in bytes) for each user task


void Idle()
{
	for(;;);
};


#define TEST_SET_8


/************************************************************************/
/*					TEST_SET_0 is an empty template						*/
/************************************************************************/
#ifdef TEST_SET_0

void t1()
{
	Task_Yield();
}

void t2()
{
	Task_Yield();
}

void test()
{
	Task_Create(t1, TASK_STACK_SIZE, 1, 0);
	Task_Create(t2, TASK_STACK_SIZE, 1, 0);
}

#endif




/************************************************************************/
/*				Test 1: Task Suspension, Resume, Sleep, Yield		    */
/************************************************************************/
#ifdef TEST_SET_1

PID Ping_PID, Pong_PID;

void Ping()
{
	for(;;)
	{
		PORTB |= LED_PIN_MASK;		//Turn on onboard LED
		printf("A!\n");
		Task_Sleep(50);
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
		Task_Suspend(Pong_PID);
		Task_Yield();
		
		Task_Sleep(300);
		printf("RESUMING PONG!\n");
		Task_Resume(Pong_PID);
		Task_Yield();
	}
}

void suicide_task()
{
	Task_Sleep(500);
	printf("SUICIDE TASK IS TERMINATING \n");
	Task_Terminate();
}

void test()
{
	DDRB = LED_PIN_MASK;			//Set pin 13 as output
	Ping_PID = Task_Create(Ping, TASK_STACK_SIZE, 6, 210);
	Pong_PID = Task_Create(Pong, TASK_STACK_SIZE, 6, 205);
	Task_Create(Peng, TASK_STACK_SIZE, 6, 205);
	
	Task_Create(suspend_pong, TASK_STACK_SIZE, 4, 0);
	Task_Create(suicide_task, TASK_STACK_SIZE, 1, 0);
}


#endif




/************************************************************************/
/*						Test 2: Priority			                    */
/************************************************************************/

#ifdef TEST_SET_2

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

void test()
{
	//These tasks tests priority scheduling
	Task_Create(priority1, TASK_STACK_SIZE, 1, 0);
	Task_Create(priority2, TASK_STACK_SIZE, 2, 0);
	Task_Create(priority3, TASK_STACK_SIZE, 3, 0);
}

#endif




/************************************************************************/
/*					Test 3: Starvation Prevention		                */
/************************************************************************/

#ifdef TEST_SET_3

void ps1()
{
	for(;;)
	{
		puts("A");
		Task_Yield();
	}

}

void ps2()
{
	int i;
	
	for(;;)
	{
		for(i=0; i<50; i++)
			puts("B");
			
		Task_Yield();
	}
}

void ps3()
{
	int i;
	
	for(;;)
	{
		for(i=0; i<50; i++)
			puts("c");
			
		Task_Yield();
	}
}

void ps4()
{
	for(;;)
	{
		puts("D");
		Task_Yield();
	}
}

void test()
{
	//These tasks tests priority scheduling
	Task_Create(ps1, TASK_STACK_SIZE, 1, 0);
	Task_Create(ps2, TASK_STACK_SIZE, 2, 0);
	Task_Create(ps3, TASK_STACK_SIZE, 3, 0);
	Task_Create(ps4, TASK_STACK_SIZE, 4, 0);
}

#endif



/************************************************************************/
/*					Test 4: Preemptive Scheduling		                */
/************************************************************************/

#ifdef TEST_SET_4

void ps1()
{
	for(;;)
	{
		puts("A");
	}

}

void ps2()
{
	for(;;)
	{
		puts("B");
	}
}

void ps3()
{
	for(;;)
	{
		puts("C");
	}
}

void ps4()
{
	for(;;)
	{
		puts("D");
	}
}

void test()
{
	//These tasks tests priority scheduling
	Task_Create(ps1, TASK_STACK_SIZE, 1, 0);
	Task_Create(ps2, TASK_STACK_SIZE, 1, 0);
	Task_Create(ps3, TASK_STACK_SIZE, 1, 0);
	Task_Create(ps4, TASK_STACK_SIZE, 1, 0);
}

#endif




/************************************************************************/
/*							Test 5: Semaphores	                        */
/************************************************************************/

#ifdef TEST_SET_5

SEMAPHORE s1;

void sem_task0()		//producer
{
	while(1)
	{
		printf("GIVING 2...\n");
		Semaphore_Give(s1, 5);
		printf("***GAVE 5\n");
		Task_Sleep(500);
	}
}

void sem_task1()		//consumer
{
	while(1)
	{
		printf("GETTING 2...\n");
		Semaphore_Get(s1, 2);
		printf("GOT 2\n");
		Task_Sleep(50);
	}
}

void test()
{
	s1 = Semaphore_Create(5, 0);
	Task_Create(sem_task0, TASK_STACK_SIZE, 4, 0);
	Task_Create(sem_task1, TASK_STACK_SIZE, 5, 0);
	
	Task_Create(sem_task1, TASK_STACK_SIZE, 5, 0);
	Task_Create(sem_task1, TASK_STACK_SIZE, 5, 0);
	Task_Create(sem_task1, TASK_STACK_SIZE, 5, 0);
	Task_Create(sem_task1, TASK_STACK_SIZE, 5, 0);
	Task_Create(sem_task1, TASK_STACK_SIZE, 5, 0);
}

#endif


/************************************************************************/
/*							Test 6: Events			                    */
/************************************************************************/

#ifdef TEST_SET_6

EVENT e1;

void event_wait_test()
{
	//Test normal signaling
	e1 = Event_Create();
	printf("Waiting for event e1...\n");
	Event_Wait(e1);
	printf("Signal for event e1 received!\n");
	
	e1 = Event_Create();
	printf("\n");
	Task_Sleep(200);
	
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

void test()
{
	Task_Create(event_wait_test, TASK_STACK_SIZE, 4, 0);
	Task_Create(event_signal_test, TASK_STACK_SIZE, 5, 0);
}

#endif

/************************************************************************/
/*						Test 7: Event Groups				            */
/************************************************************************/

#ifdef TEST_SET_7

EVENT_GROUP eg1;

void egt1()
{
	printf("T1 waking up event 0...\n");
	Event_Group_Set_Bits(eg1, (1<<0));
	
	Task_Sleep(100);
	printf("T1 waking up event 6...\n");
	Event_Group_Set_Bits(eg1, (1<<6));
	Task_Sleep(100);
	
	printf("T1 waking up event 9...\n");
	Event_Group_Set_Bits(eg1, (1<<9));
	Task_Sleep(100);
	
	printf("T1 waking up event 3...\n");
	Event_Group_Set_Bits(eg1, (1<<3));
	
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
	
	Task_Terminate();
	//for(;;)	Task_Yield();
}

void egt4()
{
	printf("T4 waiting for event 10...\n");
	Event_Group_Wait_Bits(eg1, (1<<10), 0, 500);
	printf("T4 Timed out!\n");
	
	Task_Terminate();
	//for(;;)	Task_Yield();

}

void test()
{
	eg1 = Event_Group_Create();
	
	Task_Create(egt1, TASK_STACK_SIZE, 1, 0);
	Task_Create(egt2, TASK_STACK_SIZE, 1, 0);
	Task_Create(egt3, TASK_STACK_SIZE, 1, 0);
	Task_Create(egt4, TASK_STACK_SIZE, 1, 0);
}

#endif


/************************************************************************/
/*						Test 8: Basic Mutex			                    */
/************************************************************************/

//Note: This test requires preemptive scheduling to be enabled

#ifdef TEST_SET_8

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


void test()
{
    m1 = Mutex_Create();

    Task_Create(mut_t1, TASK_STACK_SIZE, 1, 0);
    Task_Create(mut_t2, TASK_STACK_SIZE, 2, 0);
}

#endif




/************************************************************************/
/*				Test 9: Mutex with Priority Inheritance	                */
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

#ifdef TEST_SET_9

MUTEX mut;

void task_r()
{
	printf("r: terminating\n");
	Task_Terminate();
}

void task_q()
{
	printf("q: hello, gonna create R\n");

	Task_Create(task_r, TASK_STACK_SIZE, 2, 0);
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
	Task_Create(task_q, TASK_STACK_SIZE, 1, 0);
	Task_Yield();
	printf("p: gonna unlock mut\n");
	Mutex_Unlock(mut);
	Task_Yield();
	Task_Terminate();
}

void test()
{
	mut = Mutex_Create();
	Task_Create(task_p, TASK_STACK_SIZE, 3, 0);
}

#endif


/************************************************************************/
/*				Test 10: Basic Mailbox Send/Recv (async)	            */
/************************************************************************/
#ifdef TEST_SET_10

typedef struct {
	int a;
	unsigned int b;
	char* c;
}Sample_Msg;


MAILBOX mb;

void t1()
{
	Sample_Msg msg1 ;
		
	msg1.a = 11;
	msg1.b = -1-1;
	msg1.c = "aaa";
	printf("T1: Sending Mail 1...\n");
	Mailbox_Send(mb, &msg1, sizeof(Sample_Msg));
	
	msg1.a = 22;
	msg1.b = -1-2;
	msg1.c = "bbb";
	printf("T1: Sending Mail 2...\n");
	Mailbox_Send(mb, &msg1, sizeof(Sample_Msg));
	
	msg1.a = 33;
	msg1.b = -1-3;
	msg1.c = "ccc";
	printf("T1: Sending Mail 3...\n");
	Mailbox_Send(mb, &msg1, sizeof(Sample_Msg));
	
	Task_Terminate();
}

void t2()
{
	int i, count;
	MAIL r1;
	Sample_Msg *msg1;
	
	Task_Sleep(250);
	printf("\n******\n");
	count = Mailbox_Check(mb);
	printf("T2: Mailbox has %d unread mails!\n", count);
	
	for(i=0; i<count; i++)
	{
		Mailbox_Recv(mb, &r1);
		printf("T2: Mail from %d, size %d\n", r1.source, r1.size);
		msg1 = r1.ptr;
		printf("a: %d, b: %u, c: %s\n", msg1->a, msg1->b, msg1->c);
		Mailbox_Destroy_Mail(&r1);
	}
	
	Task_Terminate();
		
}

void test()
{
	mb = Mailbox_Create(10);
	
	Task_Create(t1, TASK_STACK_SIZE, 1, 0);
	Task_Create(t2, TASK_STACK_SIZE, 1, 0);
}

#endif


/************************************************************************/
/*				Test 11: Basic Mailbox Send/Recv (async)	            */
/************************************************************************/
#ifdef TEST_SET_11

typedef struct {
	int a;
	unsigned int b;
	char* c;
}Sample_Msg;


MAILBOX mb;

void t1()
{
	Sample_Msg msg1 ;
	
	msg1.a = 11;
	msg1.b = -1-1;
	msg1.c = "aaa";
	printf("T1: Sending Mail 1...\n");
	Mailbox_Send(mb, &msg1, sizeof(Sample_Msg));
	
	msg1.a = 22;
	msg1.b = -1-2;
	msg1.c = "bbb";
	printf("T1: Sending Mail 2...\n");
	Mailbox_Send(mb, &msg1, sizeof(Sample_Msg));
	
	msg1.a = 33;
	msg1.b = -1-3;
	msg1.c = "ccc";
	printf("T1: Sending Mail 3...\n");
	Mailbox_Send(mb, &msg1, sizeof(Sample_Msg));
	
	Task_Terminate();
}

void t2()
{
	int i, count;
	MAIL r1;
	Sample_Msg *msg1;
	
	Task_Sleep(250);
	printf("\n******\n");
	count = Mailbox_Check(mb);
	printf("T2: Mailbox has %d unread mails!\n", count);
	
	for(i=0; i<count; i++)
	{
		Mailbox_Recv(mb, &r1);
		printf("T2: Mail from %d, size %d\n", r1.source, r1.size);
		msg1 = r1.ptr;
		printf("a: %d, b: %u, c: %s\n", msg1->a, msg1->b, msg1->c);
		Mailbox_Destroy_Mail(&r1);
	}
	
	Task_Terminate();
	
}

void test()
{
	mb = Mailbox_Create(10);
	
	Task_Create(t1, TASK_STACK_SIZE, 1, 0);
	Task_Create(t2, TASK_STACK_SIZE, 1, 0);
}

#endif



/************************************************************************/
/*					Test 12: Mailbox Blocking Send						*/
/************************************************************************/
#ifdef TEST_SET_12

typedef struct {
	int a;
	unsigned int b;
	char* c;
}Sample_Msg;


MAILBOX mb;

void sender1()
{
	Sample_Msg msg1 = {.a = 11, .b = -1-1, .c = "aaa"};
		
	while(1)
	{
		printf("T1: Sending Mail...\n");
		Mailbox_Send_Blocking(mb, &msg1, sizeof(Sample_Msg), 0);
		Task_Yield();
	}
}

void sender2()
{
	Sample_Msg msg1 = {.a = 22, .b = -1-2, .c = "bbb"};
	
	while(1)
	{
		printf("T2: Sending Mail...\n");
		Mailbox_Send_Blocking(mb, &msg1, sizeof(Sample_Msg), 0);
		Task_Yield();
	}
}

void sender3()
{
	Sample_Msg msg1 = {.a = 33, .b = -1-3, .c = "ccc"};
	
	while(1)
	{
		printf("T3: Sending Mail...\n");
		Mailbox_Send_Blocking(mb, &msg1, sizeof(Sample_Msg), 0);
		Task_Yield();
	}
}

void receiver()
{
	int i, count;
	MAIL r1;
	Sample_Msg *msg1;
	
	Task_Sleep(250);
	
	while(1)
	{
		count = Mailbox_Check(mb);
		if(!count)
		{
			Task_Sleep(250);
			continue;
		}
		
		
		printf("\nT2: Mailbox has %d unread mails!\n", count);
		printf("******\n");
		
		for(i=0; i<count; i++)
		{
			Mailbox_Recv(mb, &r1);
			printf("Mail from %d, size %d\n", r1.source, r1.size);
			msg1 = r1.ptr;
			printf("a: %d, b: %u, c: %s\n", msg1->a, msg1->b, msg1->c);
			printf("\n");
			
			Mailbox_Destroy_Mail(&r1);
		}
		
		printf("******\n\n");
		
	}	
}

void test()
{
	mb = Mailbox_Create(3);
	
	Task_Create(sender1, TASK_STACK_SIZE, 1, 0);
	Task_Create(sender2, TASK_STACK_SIZE, 1, 0);
	Task_Create(sender3, TASK_STACK_SIZE, 1, 0);
	Task_Create(receiver, TASK_STACK_SIZE, 2, 0);
}

#endif



/************************************************************************/
/*					Test 13: Mailbox Blocking Recv						*/
/************************************************************************/
#ifdef TEST_SET_13

typedef struct {
	int a;
	unsigned int b;
	char* c;
}Sample_Msg;


MAILBOX mb;

void sender()
{
	int i;
	Sample_Msg msg[3] = {{.a = 11, .b = -1-1, .c = "aaa"},
						{.a = 22, .b = -1-2, .c = "bbb"},
						{.a = 33, .b = -1-3, .c = "ccc"}};
	
	Task_Sleep(300);
	
	while(1)
	{
		for(i=0; i<3; i++)
		{
			printf("***Sending Mail...\n");
			Mailbox_Send(mb, &msg[i], sizeof(Sample_Msg));
		}
		
		Task_Sleep(300);
	}
}


void receiver1()
{
	MAIL r1;
	Sample_Msg *msg1;
	
	while(1)
	{
		if(!Mailbox_Check(mb))
			Task_Yield();

		Mailbox_Recv_Blocking(mb, &r1, 0);
		printf("R1: Mail from %d, size %d\n", r1.source, r1.size);
		msg1 = r1.ptr;
		printf("a: %d, b: %u, c: %s\n", msg1->a, msg1->b, msg1->c);
		printf("\n");
		
		Mailbox_Destroy_Mail(&r1);
		Task_Yield();
	}
}


void receiver2()
{
	MAIL r1;
	Sample_Msg *msg1;
	
	while(1)
	{
		if(!Mailbox_Check(mb))
		Task_Yield();

		Mailbox_Recv_Blocking(mb, &r1, 0);
		printf("R2: Mail from %d, size %d\n", r1.source, r1.size);
		msg1 = r1.ptr;
		printf("a: %d, b: %u, c: %s\n", msg1->a, msg1->b, msg1->c);
		printf("\n");
		
		Mailbox_Destroy_Mail(&r1);
		Task_Yield();
	}
}


void receiver3()
{
	MAIL r1;
	Sample_Msg *msg1;
	
	while(1)
	{
		if(!Mailbox_Check(mb))
		Task_Yield();

		Mailbox_Recv_Blocking(mb, &r1, 0);
		printf("R3: Mail from %d, size %d\n", r1.source, r1.size);
		msg1 = r1.ptr;
		printf("a: %d, b: %u, c: %s\n", msg1->a, msg1->b, msg1->c);
		printf("\n");
		
		Mailbox_Destroy_Mail(&r1);
		Task_Yield();
	}
}

void test()
{
	mb = Mailbox_Create(3);
	
	Task_Create(receiver1, TASK_STACK_SIZE, 2, 0);
	Task_Create(receiver2, TASK_STACK_SIZE, 2, 0);
	Task_Create(receiver3, TASK_STACK_SIZE, 2, 0);
	Task_Create(sender, TASK_STACK_SIZE, 1, 0);
}

#endif






/************************************************************************/
/*						Entry point for application		                */
/************************************************************************/

void main()
{
	OS_Init();
		
	test();

	OS_Start();
}
