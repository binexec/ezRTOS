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


/************************************************************************/
/*						EVENT GROUP OPERATIONS                          */
/************************************************************************/

unsigned int Kernel_Create_Event_Group(void)
{
	int i;
	
	//Make sure we're not exceeding the max
	if(Event_Group_Count >= MAXEVENTGROUP)
	{
		#ifdef DEBUG
		printf("Task_Create: Failed to create task. The system is at its process threshold.\n");
		#endif
		
		err = MAX_EVENTG_ERR;
		if(KernelActive)
			Current_Process->request_ret = 0;
		return 0;
	}
	
	//Finding a free slot
	for(i=0; i<MAXEVENTGROUP; i++)
		if(Event_Group[i].id == 0)
			break;
	
	++Event_Group_Count;
	Event_Group[i].id = ++Last_Event_Group_ID;
	Event_Group[i].events = 0;
	
	err = NO_ERR;
	if(KernelActive)
		Current_Process->request_ret = Event_Group[i].id;
	return Event_Group[i].id;
}


extern volatile PD Process[MAXTHREAD];

void Kernel_Event_Group_Set_Bits()
{
	//Request args for the kernel call
	#define req_event_id		Current_Process->request_args[0]
	#define req_bits_to_set		Current_Process->request_args[1]
	
	int i;
	unsigned int current_events;
	EVENT_GROUP_TYPE *eg = findEventGroupByID(req_event_id);
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", req_event_id);
		err = EVENTG_NOT_FOUND_ERR;
		return;
	}
	
	eg->events |= req_bits_to_set;
	
	/*Wake up any tasks waiting for this event group*/
	
	#define ps_eventgroup_id	Process[i].request_args[0]
	#define ps_bits_waiting		Process[i].request_args[1]
	#define ps_wait_all_bits	Process[i].request_args[2]
	
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
			
			if(current_events > 0 && ps_wait_all_bits == 0)
			{
				//printf("Set_Bits: SOME events are ready for task %d!\n", Process[i].pid);
				Process[i].state = READY;
				continue;
			}
				
			if(current_events == ps_bits_waiting)
			{
				//printf("Set_Bits: ALL events are ready for task %d!\n", Process[i].pid);
				Process[i].state = READY;
			}
			
		}
	}
	
	#undef req_event_id		
	#undef req_bits_to_set	
	#undef ps_eventgroup_id	
	#undef ps_bits_waiting
	#undef ps_wait_all_bits
}

void Kernel_Event_Group_Clear_Bits()
{
	//Request args for the kernel call
	#define req_event_id		Current_Process->request_args[0]
	#define req_bits_to_clear	Current_Process->request_args[1]
	
	EVENT_GROUP_TYPE *eg = findEventGroupByID(req_event_id);
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", req_event_id);
		err = EVENTG_NOT_FOUND_ERR;
		return;
	}
	
	eg->events &= (~req_bits_to_clear);
	
	#undef req_event_id
	#undef req_bits_to_clear
}

void Kernel_Event_Group_Wait_Bits()
{
	//Request args for the kernel call
	#define req_event_id		Current_Process->request_args[0]
	#define req_bits_to_wait	Current_Process->request_args[1]
	#define req_wait_all_bits	Current_Process->request_args[2]
	
	EVENT_GROUP_TYPE *eg = findEventGroupByID(req_event_id);
	unsigned int current_events;
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", req_event_id);
		return;
	}
	
	current_events = eg->events & req_bits_to_wait;
		
	//No need to wait if some event bits are set, but we're not requiring to wait for all event bits to be set
	if(current_events > 0 && !req_wait_all_bits)
		return;
	
	//No need to wait If all event bits are already set
	if(current_events == req_bits_to_wait)
		return;
	
	//If the event bits are not yet ready, put the process in WAIT_EVENTG state
	Current_Process->state = WAIT_EVENTG;
	
	#undef req_event_id
	#undef req_bits_to_wait
	#undef req_wait_all_bits
}

unsigned int Kernel_Event_Group_Get_Bits()
{
	//Request args for the kernel call
	#define req_event_id		Current_Process->request_args[0]
	
	EVENT_GROUP_TYPE *eg = findEventGroupByID(req_event_id);
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", req_event_id);
		err = EVENTG_NOT_FOUND_ERR;
		return 0;
	}
	
	Current_Process->request_ret = eg->events;
	return eg->events;
	
	#undef req_event_id
}