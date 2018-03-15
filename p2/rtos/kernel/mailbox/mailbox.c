#include "mailbox.h"
#include <stdlib.h>		//Remove once kmalloc is used

volatile PtrList MailboxList;
volatile unsigned int Mailbox_Count;
volatile unsigned int Last_MailboxID;



void reset_mailbox(void)
{
	Mailbox_Count = 0;
	Last_MailboxID = 0;
	
	MailboxList.ptr = NULL;
	MailboxList.next = NULL;
}


MAILBOX_TYPE* findMailboxByID(MAILBOX mb)
{
	PtrList *i;
	MAILBOX_TYPE *mb_i;
	
	//Ensure the request event ID is > 0
	if(mb <= 0)
	{
		#ifdef DEBUG
		printf("findMailboxByID: The specified event ID is invalid!\n");
		#endif
		err = INVALID_ARG_ERR;
		return NULL;
	}
	
	for(i = &MailboxList; i; i = i->next)
	{
		mb_i = (MAILBOX_TYPE*)i->ptr;
		if (mb_i->id == mb)
			return mb_i;
	}
	
	err = OBJECT_NOT_FOUND_ERR;
	return NULL;
}





/************************************************************************/
/*							Mailbox Creation		                    */
/************************************************************************/

MAILBOX Kernel_Create_Mailbox_Direct(unsigned int capacity)
{
	MAILBOX_TYPE* mb;
	
	//Make sure the system's events are not at max
	if(Mailbox_Count >= MAXMAILBOX)
	{
		#ifdef DEBUG
		printf("Kernel_Create_Mailbox: Failed to create Mailbox. The system is at its max event threshold.\n");
		#endif
		
		err = MAX_OBJECT_ERR;
		return 0;
	}
	
	//Create a new Event object
	mb = malloc(sizeof(MAILBOX_TYPE));
	ptrlist_add(&MailboxList, mb);
	++Mailbox_Count;
	
	mb->id = ++Last_MailboxID;
	mb->capacity = capacity;
	mb->mails = new_ptr_queue();
	
	#ifdef DEBUG
	printf("Kernel_Create_Mailbox: Created Mailbox %d!\n", Last_MailboxID);
	#endif
	
	err = NO_ERR;
	return mb->id;
}

void Kernel_Create_Mailbox(void)
{
	#define req_capacity		Current_Process->request_args[0]
	
	Current_Process->request_ret = Kernel_Create_Mailbox_Direct(req_capacity);
	
	#undef req_capacity
}

void Kernel_Destroy_Mailbox(void)
{
	#define req_mb_id		Current_Process->request_args[0]
		
	PtrList *i;
	MAILBOX_TYPE *mb;
		
	//Find the corresponding Semaphore object in the semaphore list
	for(i = &MailboxList; i; i = i->next)
	{
		mb = (MAILBOX_TYPE*)i->ptr;
		if (mb->id == req_mb_id)
		break;
	}

	if(!mb)
	{
		#ifdef DEBUG
		printf("Kernel_Destroy_Mailbox: The requested Mailbox %d was not found!\n", req_mb_id);
		#endif
		err = OBJECT_NOT_FOUND_ERR;
		return;
	}
	
	free_queue(&mb->mails);					//Destroy all pending mail 
	free(mb);
	ptrlist_remove(&MailboxList, i);
	--Mailbox_Count;
		
	err = NO_ERR;
	
	#undef req_mb_id
}


/************************************************************************/
/*							Mailbox Operations		                   */
/************************************************************************/


int Kernel_Mailbox_Check_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0]
	
	return 0;
}

int Kernel_Mailbox_Send_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0]
	#define req_mail		Current_Process->request_args[1]
	
	return 0;
	
	#undef req_mb_id
	#undef req_mail
}

int Kernel_Mailbox_Get_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0]
	#define req_mail		Current_Process->request_args[1]
			
	return 0;
	
	#undef req_mb_id
	#undef req_mail
}

int Kernel_Mailbox_Wait_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0]
	#define req_mail		Current_Process->request_args[1]
	//req_timeout is also used
		
	return 0;
	
	#undef req_mb_id
	#undef req_mail
}
