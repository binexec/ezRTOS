#ifndef HW_H_
#define HW_H_

//Let the current code enter/exist an atomic, uninterrupted state
#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)

/*Context Switching functions defined in cswitch.s*/
extern void CSwitch();
extern void Enter_Kernel();				//Subroutine for entering into the kernel to handle the requested task
extern void Exit_Kernel();				



#endif /* HW_H_ */