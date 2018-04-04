# ezRTOS

ezRTOS is a real-time operating system written in C, targetting primarily microcontrollers. The RTOS aimes to provide common features expected in a RTOS with minimized platform dependency, and an easy to use front-end interface for the programmer.

A complete ezRTOS build includes the following list of features and components:
- Multitasking scheduler, with optional time-based preemption and starvation prevention
- Tasks controlls, such as suspension/resumption, sleep, and yield. 
- Events and Event Groups
- Semaphores
- Mutex with priority inheritence
- Mailbox for interprocess communications.


## Prerequisites
ezRTOS was developed using [**Atmel Studios 7.0**](https://www.microchip.com/avr-support/atmel-studio-7), but should work with further versions too. 
Simply open _ezRTOS.atsln_ with Atmel Studios and the IDE will prompt you to install any missing libraries and headers automatically.

If you simply compile and upload the RTOS without adding your own functionalities, test case 0 from _rtos_test.c_ will start running.
When you're ready to integrate ezRTOS into your project, be sure to delete this file before start writing your own entry point.

## Basic Usage
As usual with any C programs, the _main()_ function is the entry point even with ezRTOS used. Before any OS functions can be used however, your main() function must initializes the OS, create some tasks and other objects needed by the tasks, and then start the OS. The program flow for your main() should be structured as the following:

```
void main()
{
    /*Initialization for your project's own resources here...*/
    
    OS_Init();      //Initializes the OS
	
    /*Create some tasks, other kernel objects here...*/

	OS_Start();     //Start the OS. Created tasks will start running 
}
```

### Creating a Task
To create a task, consider the function **Task_Create**
>PID Task_Create(taskfuncptr f, size_t stack_size, PRIORITY py, int arg);

A task is fundamentally represented by a _void function_ **f**. This function should be running in a continuous loop, or else the task will terminate when the function returns. It's recommended for each task's main function (and its associated helper functions and variables) isolated seperate C file from other tasks, for ease of organization.

Each task in the RTOS has its own stack to store its variables. The argument **stack_size** defines how many bytes should the task's stack be.

A priority **py** associated with each task, and ranges from 0 (highest) to 10 (lowest) by default. The priority of a task is the main deciding factor to choose which task to run next by the kernel scheduler.

A task can have an optional user argument **arg**. The purpose of this value is determined by the user, and is not considered by the kernel or the scheduler.

If a task has been successfully created, the function Task_Create will return a positive value as the **PID**. You must save this return value if you're planning to have this task interacting with other OS components later. If a value of 0 was returned, task creation has failed.

For more information on all available operations for the OS, tasks, and its other components, see _os.h_ for more detail.

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
