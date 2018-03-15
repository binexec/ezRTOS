#ifndef TASK_H_
#define TASK_H_

#include "../kernel_shared.h"
#include "../hardware/cpuarch.h"


PID Kernel_Create_Task(void);
PID Kernel_Create_Task_Direct(taskfuncptr f, size_t stack_size, PRIORITY py, int arg);
void Task_Reset(void);
void Kernel_Suspend_Task(void);
void Kernel_Resume_Task(void); 
void Kernel_Sleep_Task(void);
void Kernel_Terminate_Task(void);


/*Variables shared with the main kernel module*/
extern volatile PtrList ProcessList;					//Be responsible when directly accessing Process Descriptors
extern volatile unsigned int Task_Count;	



#endif /* TASK_H_ */