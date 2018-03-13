#include "event_group.h"
#include <stdlib.h>		//Remove once kmalloc is used

volatile static PtrList EventGroupList;
volatile unsigned int Event_Group_Count;
volatile unsigned int Last_Event_Group_ID;	


EVENT_GROUP_TYPE* findEventGroupByID(EVENT_GROUP eg)
{
	PtrList *i;
	EVENT_GROUP_TYPE *eg_i;
		
	for(i = &EventGroupList; i; i = i->next)
	{
		eg_i = (EVENT_GROUP_TYPE*)i->ptr;
		if (eg_i->id == eg)
			return eg_i;
	}
		
	return NULL;
}


void Event_Group_Reset(void)
{
	Event_Group_Count = 0;
	Last_Event_Group_ID = 0;
	
	EventGroupList.ptr = NULL;
	EventGroupList.next = NULL;
}


/************************************************************************/
/*						EVENT GROUP CREATION	                      */
/************************************************************************/

unsigned int Kernel_Create_Event_Group(void)
{
	EVENT_GROUP_TYPE *eg;
	
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
	
	//Create a new Event Group object
	eg = malloc(sizeof(EVENT_GROUP_TYPE));
	ptrlist_add(&EventGroupList, eg);	
	++Event_Group_Count;
	
	eg->id = ++Last_Event_Group_ID;
	eg->events = 0;
	
	err = NO_ERR;
	if(KernelActive)
		Current_Process->request_ret = eg->id;
		
	return eg->id;
}


void Kernel_Destroy_Event_Group(void)
{
	#define req_eg_id		Current_Process->request_args[0]
	
	PtrList *i;
	EVENT_GROUP_TYPE *eg;
		
	//Find the corresponding Semaphore object in the semaphore list
	for(i = &EventGroupList; i; i = i->next)
	{
		eg = (EVENT_GROUP_TYPE*)i->ptr;
		if (eg->id == req_eg_id)
			break;
	}

	if(!eg)
	{
		#ifdef DEBUG
		printf("Kernel_Destroy_Event_Group: The requested Semaphore %d was not found!\n", req_eg_id);
		#endif
		err = SEMAPHORE_NOT_FOUND_ERR;
		return;
	}
	
	/*Should we check and make sure the wait queue is empty first?*/
	
	free(eg);
	ptrlist_remove(&EventGroupList, i);
	
	err = NO_ERR;
	#undef req_eg_id
}


/************************************************************************/
/*						EVENT GROUP OPERATIONS                          */
/************************************************************************/


void Kernel_Event_Group_Set_Bits()
{
	//Request args for the kernel call
	#define req_event_id		Current_Process->request_args[0]
	#define req_bits_to_set		Current_Process->request_args[1]
	
	EVENT_GROUP_TYPE *eg = findEventGroupByID(req_event_id);
	
	PtrList* i;
	PD* process_i;
	unsigned int current_events;
	
	if(eg == NULL)
	{
		printf("Event_Group_Set_Bits: Event group %d was not found!\n", req_event_id);
		err = EVENTG_NOT_FOUND_ERR;
		return;
	}
	
	eg->events |= req_bits_to_set;

	/* Wake up any process that's currently waiting for this event group if:
			-some of its events are ready, and it doesn't require to wait for all to be ready 
			-OR all of its events being waited on are ready 
		
		Maybe use a list instead of directly accessing PDs?
	*/
	
	#define ps_eventgroup_id	process_i->request_args[0]
	#define ps_bits_waiting		process_i->request_args[1]
	#define ps_wait_all_bits	process_i->request_args[2]
	
	for(i = &ProcessList; i; i = i->next)
	{
		process_i = (PD*)i->ptr;
		
		if(process_i->state == WAIT_EVENTG && ps_eventgroup_id == eg->id)						
		{
			current_events = ps_bits_waiting & eg->events;
			
			if((current_events > 0 && !ps_wait_all_bits) || (current_events == ps_bits_waiting))
				process_i->state = READY;
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
	Kernel_Request_Cswitch = 1;
	
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