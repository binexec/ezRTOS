#include "cpuarch.h"



/************************************************************************/
/*						Initialize task workspace                       */
/************************************************************************/

//sp_ptr is a pointer to sp, the actual stack pointer for the new task.
//sp points to the bottom of the memory stack for the new task to be created

void Kernel_Init_Task_Stack(unsigned char **sp_ptr, voidfuncptr f)
{
	unsigned char *sp = *sp_ptr;
	
	//Store the address of Task_Terminate at the bottom of stack to protect against stack underrun (eg, if the task is not a loop).
	*(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00;

	//Place the address of the task's main function at bottom of stack for execution
	*(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00;
	
	//Allocate the stack with enough memory spaces to save the registers needed for ctxswitch
	*sp_ptr = sp - 34;
}