#include "event.h"
#include <stdlib.h>		//Remove once kmalloc is used

volatile static PtrList EventList;				//Contains all the event objects 
volatile unsigned int Event_Count;				//Number of events created so far.
volatile unsigned int Last_EventID;				//Last (also highest) EVENT value created so far.


void Event_Reset()
{	
	Event_Count = 0;
	Last_EventID = 0;
	
	EventList.ptr = NULL;
	EventList.next = NULL;
}



EVENT_TYPE* findEventByEventID(EVENT e)
{
	PtrList *i;
	EVENT_TYPE *event_i;
	
	//Ensure the request event ID is > 0
	if(e <= 0)
	{
		#ifdef DEBUG
		printf("findEventByID: The specified event ID is invalid!\n");
		#endif
		err = INVALID_ARG_ERR;
		return NULL;
	}
	
	for(i = &EventList; i; i = i->next)
	{
		event_i = (EVENT_TYPE*)i->ptr;
		if (event_i->id == e)
			return event_i;
	}
	
	err = OBJECT_NOT_FOUND_ERR;
	return NULL;
}


/************************************************************************/
/*							EVENT Creation			                    */
/************************************************************************/

//We do not need a separate Kernel_Create_Event_Direct function, since creating an event object requires no parameters

EVENT Kernel_Create_Event(void)
{
	EVENT_TYPE* e;
	
	//Make sure the system's events are not at max
	if(Event_Count >= MAXEVENT)
	{
		#ifdef DEBUG
		printf("Event_Init: Failed to create Event. The system is at its max event threshold.\n");
		#endif
		
		err = MAX_OBJECT_ERR;
		if(KernelActive)
			Current_Process->request_retval = 0;
		return 0;
	}
	
	//Create a new Event object
	e = malloc(sizeof(EVENT_TYPE));
	ptrlist_add(&EventList, e);
	++Event_Count;
	
	//Assign a new unique ID to the event. Note that the smallest valid Event ID is 1.
	e->id = ++Last_EventID;
	e->owner = 0;
	
	#ifdef DEBUG
	printf("Event_Init: Created Event %d!\n", Last_EventID);
	#endif
	
	err = NO_ERR;
	if(KernelActive)	
		Current_Process->request_retval = e->id;
		
	return e->id;
}

static void Kernel_Destroy_Event_Internal(EVENT_TYPE *e)
{
	//Destroy the event object
	e->owner = 0;
	e->count = 0;
	e->id = 0;
	
	free(e);
	ptrlist_remove(&EventList, ptrlist_find(&EventList, e));
	--Event_Count;
}


/************************************************************************/
/*							EVENT Operations		                   */
/************************************************************************/


void Kernel_Wait_Event(void)
{
	EVENT_TYPE* e = findEventByEventID(Current_Process->request_args[0].val);
	
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
		err = OBJECT_NOT_FOUND_ERR;
		return;
	}
	
	//Has this event been signaled already? If yes, "consume" event and keep executing the same task
	if(e->count > 0)
	{
		Kernel_Destroy_Event_Internal(e);
		return;
	}
	
	//Set the owner of the requested event to the current task and put it into the WAIT EVENT state
	e->owner = Current_Process->pid;
	Current_Process->state = WAIT_EVENT;
	Kernel_Request_Cswitch = 1;
	
	err = NO_ERR;
}

void Kernel_Signal_Event(void)
{
	EVENT_TYPE* e = findEventByEventID(Current_Process->request_args[0].val);
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
	
	//If the event is unowned, no need to wake anyone up
	if(e->owner == 0)
		return;
	
	//Wake up the owner of the event by setting its state to READY if it's active. The event is "consumed"
	e_owner = findProcessByPID(e->owner);
	e_owner->state = READY;
	Kernel_Destroy_Event_Internal(e);
		
	Kernel_Request_Cswitch = 1;
}
