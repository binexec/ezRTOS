/***********************************************************************
  Kernel.h and Kernel.c contains the backend of the RTOS.
  It contains the underlying Kernel that process all requests coming in from OS syscalls.
  Most of Kernel's functions are not directly usable, but a few helpers are provided for the OS for convenience and for booting purposes.
  ***********************************************************************/

#ifndef KERNEL_H_
#define KERNEL_H_

#include "kernel_internal.h"
#include "task/task.h"
#include "hardware/hw.h"
#include "hardware/cpuarch.h"
#include "mutex/mutex.h"
#include "event/event.h"
#include "semaphore/semaphore.h"
#include "event/event_group.h"



/************************************************************************/
/*                Kernel functions accessible by the OS                 */
/************************************************************************/

/*Kernel Initialization*/
void Kernel_Reset();
void Kernel_Start();


/*Create kernel objects before */
void Kernel_Create_Task(voidfuncptr f, PRIORITY py, int arg);
int findPIDByFuncPtr(voidfuncptr f);
void print_processes();


/*Hardware Related*/
void Kernel_Tick_ISR();




/*Kernel variables accessible by the OS*/
extern volatile PD* Current_Process;
extern volatile unsigned char *KernelSp;
extern volatile unsigned char *CurrentSp;
extern volatile unsigned int KernelActive;
extern volatile ERROR_TYPE err;
extern volatile unsigned int Last_PID;




#endif /* KERNEL_H_ */