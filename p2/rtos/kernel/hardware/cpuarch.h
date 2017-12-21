#ifndef CPUARCH_H_
#define CPUARCH_H_

#include "../kernel_internal.h"



void Kernel_Init_Task_Stack(unsigned char **sp_ptr, taskfuncptr f);



#endif /* CPUARCH_H_ */