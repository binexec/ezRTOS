/***********************************************************************
  Kernel.h and Kernel.c contains the backend of the RTOS.
  It contains the underlying Kernel that process all requests coming in from OS syscalls.
  Most of Kernel's functions are not directly usable, but a few helpers are provided for the OS for convenience and for booting purposes.
  ***********************************************************************/

#ifndef KERNEL_H_
#define KERNEL_H_

#include "kernel_shared.h"
#include "task/task.h"
#include "hardware/hw.h"
#include "hardware/cpuarch.h"


/* Include optional kernel modules only if they're enabled in ../os.h */

#ifdef MUTEX_ENABLED
#include "mutex/mutex.h"
#endif

#ifdef EVENT_ENABLED
#include "event/event.h"
#endif

#ifdef SEMAPHORE_ENABLED
#include "semaphore/semaphore.h"
#endif

#ifdef EVENT_GROUP_ENABLED
#include "event/event_group.h"
#endif

#ifdef MAILBOX_ENABLED
#include "mailbox/mailbox.h"
#endif



/************************************************************************/
/*                Kernel functions accessible by the OS                 */
/************************************************************************/

/*Kernel Initialization*/
void Kernel_Reset();
void Kernel_Start();


/*Hardware Related*/
void Kernel_Tick_ISR();


/*Debug*/
void print_processes();
void print_process(PID p);



#endif /* KERNEL_H_ */