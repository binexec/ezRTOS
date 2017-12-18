#include "event_group.h"

volatile static EVENT_GROUP_TYPE Event_Group[MAXEVENTGROUP];

volatile unsigned int Event_Group_Count;
volatile unsigned int Last_Event_Group_ID;	


EVENT_GROUP_TYPE* findEventGroupByID(EVENT_GROUP e)
{
	int i;
	
	for(i=0; i<MAXEVENTGROUP; i++)
		if(Event_Group[i].id == e)
			return &Event_Group[i];
	
	return NULL;
}


void Event_Group_Reset(void)
{
	Event_Group_Count = 0;
	Last_Event_Group_ID = 0;
	
	memset(&Event_Group, 0, sizeof(EVENT_GROUP_TYPE)*MAXEVENTGROUP);
}


unsigned int Kernel_Create_Event_Group(void)
{
	int i;
	
	//Make sure we're not exceeding the max
	if(Event_Group_Count >= MAXEVENTGROUP)
	{
		return 0;
	}
	
	//Finding a free slot
	for(i=0; i<MAXEVENTGROUP; i++)
		if(Event_Group[i].id == 0)
			break;
	
	++Event_Group_Count;
	Event_Group[i].id = ++Last_Event_Group_ID;
	Event_Group[i].events = 0;
	
	return Last_Event_Group_ID;
}


extern volatile PD Process[MAXTHREAD];	

#define ps_eventgroup_id	Process[i].request_args[0]
#define ps_bits_waiting		Process[i].request_args[1]
#define ps_wait_all_bits	Process[i].request_args[2]	

void Kernel_Event_Group_Set_Bits(EVENT_GROUP e, unsigned int bits_to_set)
{
	EVENT_GROUP_TYPE *eg = findEventGroupByID(e);
	
	int i;
	unsigned int current_events;
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", e);
		return;
	}
	
	eg->events |= bits_to_set;
	
	//printf("Set_Bits: ID: %d, Current events: %d\n", eg->id, eg->events);
	
	//Wake up any tasks waiting for this event group
	for(i=0; i<MAXTHREAD; i++)
	{
		//Find all tasks currently waiting for this event group
		if(Process[i].state == WAIT_EVENTG && ps_eventgroup_id == eg->id)						
		{
			current_events = ps_bits_waiting & eg->events;
			
			/* Wake the process up if:
			*	-some of its events are ready, and it doesn't require to wait for all to be ready 
			*	-OR all of its events being waited on are ready */

			/*if((current_events > 0 && !ps_wait_all_bits) || (current_events == ps_bits_waiting))
				Process[i].state = READY;*/
			
			//printf("\t Task %d; current_event: %d; bits_waiting: %d\n\n", Process[i].pid, current_events, ps_bits_waiting);
			
			if(current_events > 0 && ps_wait_all_bits == 0)
			{
				printf("Set_Bits: SOME events are ready for task %d!\n", Process[i].pid);
				Process[i].state = READY;
				continue;
			}
				
			if(current_events == ps_bits_waiting)
			{
				printf("Set_Bits: ALL events are ready for task %d!\n", Process[i].pid);
				Process[i].state = READY;
			}
			
		}
	}
}

void Kernel_Event_Group_Clear_Bits(EVENT_GROUP e, unsigned int bits_to_clear)
{
	EVENT_GROUP_TYPE *eg = findEventGroupByID(e);
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", e);
		return;
	}
	
	eg->events &= (~bits_to_clear);
}

void Kernel_Event_Group_Wait_Bits(EVENT_GROUP e, unsigned int bits_to_wait, unsigned int wait_all_bits, TICK timeout)
{
	EVENT_GROUP_TYPE *eg = findEventGroupByID(e);
	unsigned int current_events;
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", e);
		return;
	}
	
	current_events = eg->events & bits_to_wait;
		
	//No need to wait if some event bits are set, but we're not requiring to wait for all event bits to be set
	if(current_events > 0 && !wait_all_bits)
	{
		//printf("Wait_Bits: SOME events are already ready for task %d!\n", Current_Process->pid);
		return;
	}
	
	//No need to wait If all event bits are already set
	if(current_events == bits_to_wait)
	{
		//printf("Wait_Bits: ALL events are already ready for task %d!\n", Current_Process->pid);
		return;
	}
	
	//If the event bits are not yet ready, put the process in WAIT_EVENTG state
	//printf("Wait_Bits: Task %d is now in WAIT_EVENTG...\n", Current_Process->pid);
	Current_Process->state = WAIT_EVENTG;
}

unsigned int Kernel_Event_Group_Get_Bits(EVENT_GROUP e)
{
	EVENT_GROUP_TYPE *eg = findEventGroupByID(e);
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", e);
		return 0;
	}
	
	return eg->events;
}