#include "hw.h"


#define TICK_LENG 625			//The length of a tick = 10ms, using 16Mhz clock and /256 prescsaler


/************************************************************************/
/*						Timer                                */
/************************************************************************/

ISR(TIMER1_COMPA_vect)
{
	++Tick_Count;
}


/*Sets up the timer needed for task_sleep*/
void Timer_init()
{
	/*Timer1 is configured for the task*/
	
	//Use Prescaler = 256
	TCCR1B |= (1<<CS12);
	TCCR1B &= ~((1<<CS11)|(1<<CS10));
	
	//Use CTC mode (mode 4)
	TCCR1B |= (1<<WGM12);
	TCCR1B &= ~((1<<WGM13)|(1<<WGM11)|(1<<WGM10));
	
	OCR1A = TICK_LENG;			//Set timer top comparison value to ~10ms
	TCNT1 = 0;					//Load initial value for timer
	TIMSK1 |= (1<<OCIE1A);      //enable match for OCR1A interrupt
	
	#ifdef DEBUG
	printf("Timer initialized!\n");
	#endif
}


/************************************************************************/
/*						Enable STDIO redirection                        */
/************************************************************************/

void stdio_init()
{
	uart_init();
	uart_setredir();
	printf("STDOUT->UART!\n");
}

/************************************************************************/
/*						Initialize task workspace                       */
/************************************************************************/

//"sp" is the bottom of the memory stack for the new task to be created
void Kernel_Init_Task_Stack(unsigned char *sp, voidfuncptr f)
{
	//Store the address of Task_Terminate at the bottom of stack to protect against stack underrun (eg, if the task is not a loop).
	*(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00;

	//Place the address of the task's main function at bottom of stack for execution
	*(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00;
	
	//Allocate the stack with enough memory spaces to save the registers needed for ctxswitch
	sp = sp - 34;
}