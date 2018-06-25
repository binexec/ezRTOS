# ezRTOS

ezRTOS is a real-time operating system written in C, targetting primarily microcontrollers. The RTOS aimes to provide common features expected in a RTOS with minimized platform dependency, and an easy to use front-end interface for the programmer.

A complete ezRTOS build includes the following list of features and components:
- Multitasking scheduler, with optional time-based preemption and starvation prevention
- Tasks controlls, such as suspension/resumption, sleep, and yield. 
- Events and Event Groups
- Semaphores
- Mutex with priority inheritence
- Mailbox for interprocess communications.

For more information on all available operations for the OS, tasks, and its other components, see _os.h_ for more detail.

## Prerequisites
ezRTOS was developed using [**Atmel Studios 7.0**](https://www.microchip.com/avr-support/atmel-studio-7), but should work with further versions too. 
Simply open _ezRTOS.atsln_ with Atmel Studios and the IDE will prompt you to install any missing libraries and headers automatically.

If you simply compile and upload the RTOS without adding your own functionalities, test case 0 from _rtos_test.c_ will start running.
When you're ready to integrate ezRTOS into your project, be sure to delete this file before start writing your own entry point.

## Basic Usage
To help you getting started on your first basic multi-tasking application, we will provide a simple demo program that prints alternating "Ping" and "Pong".

### Creating Your First Task
To create a task, consider the function **Task_Create**:
>PID Task_Create(taskfuncptr f, size_t stack_size, PRIORITY py, int arg);

A task is fundamentally represented by a _void function_ **f**. This function should be running in a continuous loop, or else the task will terminate when the function returns. It's recommended for each task's main function (and its associated helper functions and variables) isolated seperate C file from other tasks, for ease of organization.

Each task in the RTOS has its own stack to store its variables. The argument **stack_size** defines how many bytes should the task's stack be.

A priority **py** associated with each task, and ranges from 0 (highest) to 10 (lowest) by default. The priority of a task is the main deciding factor to choose which task to run next by the kernel scheduler.

A task can have an optional user argument **arg**. The purpose of this value is determined by the user, and is not considered by the kernel or the scheduler.

If a task has been successfully created, the function Task_Create will return a positive value as the **PID**. You must save this return value if you're planning to have this task interacting with other OS components later. If a value of 0 was returned, task creation has failed.

#### Sample Application
Below is a sample application that uses two tasks to print alternating "Ping" and "Pong" to stdout. 

```c
#include "rtos/os.h"

void ping()
{
	for(;;)
	{
		printf("Ping\n");
		Task_Yield();
	}
}

void pong()
{
	for(;;)
	{
		printf("Pong\n");
		Task_Yield(); 
	}
}

void main()
{
	PID ping_pid, pong_pid;
    
    /*Initializes the RTOS*/
    OS_Init();
	
    /*Create a task for PING and PONG. 
     -Each task has 128 bytes of stack memories allocated for local variables, 
     -Both tasks has a priority of 1.*/

	ping_pid = Task_Create(ping, 128, 1, 0);
	pong_pid = Task_Create(pong, 128, 1, 0);

    /*Ensure the tasks has been created successfully*/
    if(!ping_pid || !pong_pid)
    {
        printf("Failed to create tasks!\n");
        exit(0);
    }

    /*Start the RTOS*/
	OS_Start();
}
```

##### Observations:

* **Each tasks/thread is abstracted by a void function**. In this case, the task that prints "Ping" and "Pong" is abstracted as ping() and pong() respectively.
* **A periodic task should always be running in an infinite loop**. Otherwise, the task will be killed if its corresponding function returns.
* A task should call ```Task_Yield()``` after having completed one full iteration of work. This allows other tasks to run.
    * You could optionally enable preemptive multitasking, if you want tasks to switch forcibly by the RTOS on a timed basis. 
* As with any other typical C program, main() is the entry point. However, **the structure of main() is crucial** and must be based on the following steps:
    1. Initialize the RTOS with ```OS_Init()```
    2. Initialize other application components, if needed
    3. Create your starting Tasks, and other RTOS components needed right at the start
    4. Start the RTOS with ```OS_Start()```, and your initially created tasks will begin executing.


## Porting Guide
The default configuration provided is targetted for Atmel Atmega2560, but can be ported to other microcontrollers and architectures with ease. If the target microcontroller is an Atmega2560 for your project, no further actions are needed.

To port ezRTOS to another microcontroller of the same CPU architecture, the following porting steps are recommended:
1. Uncomment "**#define DEBUG**" in _ezRTOS/p2/rtos/os.h_. 
This enables the kernel to print out additional debug messages to aid your progress

2. Implement **stdio_init()** defined in _ezRTOS/p2/rtos/hardware/hw.c_. 
You will use this function to initialize an UART port for receiving debug messages, and then redirect _stdout_ and _stderr_ to print to the UART port. 

3. Implement **Timer_init()** defined in _ezRTOS/p2/rtos/hardware/hw.c_.
You will use this function to initialize a hardware timer to repeatedly cause an interrupt every _t_ milliseconds. 
Whenever the timer's ISR is invoked, the ISR must directly call the function **Kernel_Tick_ISR()** defined in _ezRTOS/p2/rtos/kernel/kernel.c_. Do not edit this function.

If the target microcontroller uses a different CPU architecture, the following additional steps are required. These steps are not needed for any other AVR microcontrollers.

4. Implement **Enable_Interrupt()** and **Disable_Interrupt()**, defined in  _ezRTOS/p2/rtos/hardware/cpuarch.h_
These functions allows the OS and kernel to enter and exit an atomic state uninterruptable by external sources.

5. Implement **Context Switching**
    - First implement a macro for _saving_ the current context onto the stack, as well as _restoring_ the context from the current stack
    - Then implement the function **CSwitch()** defined in _ezRTOS/p2/rtos/hardware/cpuarch.h_
    - When CSwitch()/Enter_Kernel() is called, the function must save the current running task's context onto the stack, and load the kernel's context.
    - When **Exit_Kernel()**, the opposit occurs.
    - The current implementation for context switching on AVR is done in CSwitch.s

## Todo

Currently, kernel objects and linked list nodes are dynamically allocated onto the heap using the same malloc function provided by stdlib. There are two inherent concerns, as the same memory heap is shared with the user's memory allocations:

1. Kernel objects may be corrupted if the user accidentally writes out of bound of its own allocated memory
2. If the target platform does not have a native implementation of malloc in stdlib, the RTOS is unusable

A seperate dynamic memory allocator has been implemented in the [DynMemAllocator](https://github.com/bowen-liu/DynMemAllocator) repo. By letting the RTOS manage its own memory heap, these concerns are mitigated. However, the memory allocator was developed using an x86 machine (even though the algorithm is system/architecture independant), and we have not yet tested it on AVR. The next step would be incorporating it into the RTOS.