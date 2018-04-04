#ifndef HW_H_
#define HW_H_

#include "../kernel_shared.h"
#include "cpuarch.h"

//Include any hardware libraries needed
#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart/uart.h"


/*Initializing essential hardware components*/
void Timer_init();
void stdio_init();


#endif /* HW_H_ */