#ifndef HW_H_
#define HW_H_

#include "../kernel_internal.h"

//Include any hardware libraries needed
#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart/uart.h"

//Kernel Variables needed
extern volatile unsigned int Tick_Count;		//Timer Tick counter, declared in kernel.h


//Let the current code enter/exist an atomic, uninterrupted state
#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)


/*Initializing essential hardware components*/
void Timer_init();
void stdio_init();


/*Context Switching functions defined in cswitch.s*/
extern void CSwitch();
extern void Enter_Kernel();				//Subroutine for entering into the kernel to handle the requested task
extern void Exit_Kernel();	
	

/*Other architecture dependent functions*/
void Kernel_Init_Task_Stack(unsigned char *sp, voidfuncptr f);



#endif /* HW_H_ */