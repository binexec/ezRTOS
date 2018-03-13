#ifndef EVENT_H_
#define EVENT_H_

#define EVENT_ENABLED

#include "../kernel_internal.h"

#define MAX_EVENT_SIG_MISS 1	//The maximum number of missed signals to record for an event. 0 = unlimited

//For the ease of manageability, we're making a new event data type. The old EVENT type defined in OS.h will simply serve as an identifier.
typedef struct event_type
{
	EVENT id;								//An unique identifier for this event. 0 = uninitialized
	PID owner;								//Who's currently waiting for this event this?
	unsigned int count;						//How many unhandled events has been collected?
} EVENT_TYPE;


/*Kernel variables accessible by the OS*/
extern volatile unsigned int Last_EventID;


void Event_Reset();
EVENT Kernel_Create_Event(void);
void Kernel_Wait_Event(void);
void Kernel_Signal_Event(void);
EVENT_TYPE* findEventByEventID(EVENT e);


#endif /* EVENT_H_ */