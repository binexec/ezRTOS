#include "event.h"

#include <string.h>

volatile static EVENT_TYPE Event[MAXEVENT];		//Contains all the event objects 
volatile unsigned int Event_Count;				//Number of events created so far.
volatile unsigned int Last_EventID;				//Last (also highest) EVENT value created so far.


void Event_Reset()
{
	int x = 0;
	
	Event_Count = 0;
	Last_EventID = 0;
	
	//Clear and initialize the memory used for Events
	memset(Event, 0, MAXEVENT*sizeof(EVENT_TYPE));
	for (x = 0; x < MAXEVENT; x++) {
		Event[x].id = 0;
	}
	
}



EVENT_TYPE* findEventByEventID(EVENT e)
{
	int i;
	
	//Ensure the request event ID is > 0
	if(e <= 0)
	{
		#ifdef DEBUG
		printf("findEventByID: The specified event ID is invalid!\n");
		#endif
		err = INVALID_ARG_ERR;
		return NULL;
	}
	
	//Find the requested Event and return its pointer if found
	for(i=0; i<MAXEVENT; i++)
	{
		if(Event[i].id == e)
		return &Event[i];
	}
	
	//Event wasn't found
	//#ifdef DEBUG
	//printf("findEventByEventID: The requested event %d was not found!\n", e);
	//#endif
	err = EVENT_NOT_FOUND_ERR;
	return NULL;
}


/*Only useful if our RTOS allows more than one missed event signals to be recorded*/
int getEventCount(EVENT e)
{
	EVENT_TYPE* e1 = findEventByEventID(e);
	
	if(e1 == NULL)
	return 0;
	
	return e1->count;
}


/************************************************************************/
/*                  EVENT RELATED KERNEL FUNCTIONS                      */
/************************************************************************/

EVENT Kernel_Create_Event(void)
{
	int i;
	
	//Make sure the system's events are not at max
	if(Event_Count >= MAXEVENT)
	{
		#ifdef DEBUG
		printf("Event_Init: Failed to create Event. The system is at its max event threshold.\n");
		#endif
		
		err = MAX_EVENT_ERR;
		if(KernelActive)
			Current_Process->request_ret = 0;
		return 0;
	}
	
	//Find an uninitialized Event slot
	for(i=0; i<MAXEVENT; i++)
		if(Event[i].id == 0) 
			break;
	
	//Assign a new unique ID to the event. Note that the smallest valid Event ID is 1.
	Event[i].id = ++Last_EventID;
	Event[i].owner = 0;
	++Event_Count;
	
	
	#ifdef DEBUG
	printf("Event_Init: Created Event %d!\n", Last_EventID);
	#endif
	
	err = NO_ERR;
	if(KernelActive)	
		Current_Process->request_ret = Event[i].id;
	return Event[i].id;
}

void Kernel_Wait_Event(void)
{
	EVENT_TYPE* e = findEventByEventID(Current_Process->request_args[0]);
	
	if(e == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Wait_Event: Error finding requested event!\n");
		#endif
		return;
	}
	
	//Ensure no one else is waiting for this same event
	if(e->owner > 0 && e->owner != Current_Process->pid)
	{
		#ifdef DEBUG
		printf("Kernel_Wait_Event: The requested event is already being waited by PID %d\n", e->owner);
		#endif
		err = EVENT_NOT_FOUND_ERR;
		return;
	}
	
	//Has this event been signaled already? If yes, "consume" event and keep executing the same task
	if(e->count > 0)
	{
		e->owner = 0;
		e->count = 0;
		e->id = 0;
		--Event_Count;
		return;
	}
	
	//Set the owner of the requested event to the current task and put it into the WAIT EVENT state
	e->owner = Current_Process->pid;
	Current_Process->state = WAIT_EVENT;
	err = NO_ERR;
	
	
}

void Kernel_Signal_Event(void)
{
	EVENT_TYPE* e = findEventByEventID(Current_Process->request_args[0]);
	PD *e_owner;
	
	if(e == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Signal_Event: Error finding requested event!\n");
		#endif
		return;
	}
	
	//Increment the event counter if needed
	if(MAX_EVENT_SIG_MISS == 0 || e->count < MAX_EVENT_SIG_MISS)
	e->count++;
	
	//If the event is unowned, return
	if(e->owner == 0)
	{
		#ifdef DEBUG
		printf("Kernel_Signal_Event: *WARNING* The requested event is not being waited by anyone!\n");
		#endif
		err = SIGNAL_UNOWNED_EVENT_ERR;
		return;
	}
	
	//Fetch the owner's PD and ensure it's still valid
	e_owner = findProcessByPID(e->owner);
	if(e_owner == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Signal_Event: Event owner's PID not found in global process list!\n");
		#endif
		err = PID_NOT_FOUND_ERR;
		return;
	}
	
	//Wake up the owner of the event by setting its state to READY if it's active. The event is "consumed"
	if(e_owner->state == WAIT_EVENT)
	{
		e->owner = 0;
		e->count = 0;
		e->id = 0;
		--Event_Count;
		e_owner->state = READY;
	}
	
	
}
