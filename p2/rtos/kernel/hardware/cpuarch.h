#ifndef CPUARCH_H_
#define CPUARCH_H_

#include "../kernel_shared.h"


/*Let the current code enter/exist an atomic, uninterrupted state*/
#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)



/*Context Switching functions defined in cswitch.s*/
extern void CSwitch();
extern void Enter_Kernel();						//Note that Enter_Kernel() and CSwitch() does the same thing in the current implementation
extern void Exit_Kernel();


void Kernel_Init_Task_Stack(unsigned char **sp_ptr, taskfuncptr f);



#endif /* CPUARCH_H_ */