#include "hw.h"


#define TICK_LENG 625			//The length of a tick = 10ms, using 16Mhz clock and /256 prescsaler


/************************************************************************/
/*						Timer                                */
/************************************************************************/

ISR(TIMER1_COMPA_vect)
{
	Kernel_Tick_ISR();
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

