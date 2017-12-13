#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "../kernel.h"



void Scheduler_Reset();
void Scheduler_Tick_ISR();
void Kernel_Dispatch_Next_Task();



#endif /* SCHEDULER_H_ */