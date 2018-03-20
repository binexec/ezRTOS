#include "mailbox.h"
#include <string.h>
#include <stdlib.h>		//Remove once kmalloc is used

volatile PtrList MailboxList;
volatile unsigned int Mailbox_Count;
volatile unsigned int Last_MailboxID;



void Mailbox_Reset(void)
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
		kernel_raise_error(INVALID_ARG_ERR);
		return NULL;
	}
	
	for(i = &MailboxList; i; i = i->next)
	{
		mb_i = (MAILBOX_TYPE*)i->ptr;
		if (mb_i->id == mb)
			return mb_i;
	}
	
	kernel_raise_error(OBJECT_NOT_FOUND_ERR);
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
		
		kernel_raise_error(MAX_OBJECT_ERR);
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
	
	
	return mb->id;
}


void Kernel_Create_Mailbox(void)
{
	#define req_capacity		Current_Process->request_args[0].val
	
	Current_Process->request_retval = Kernel_Create_Mailbox_Direct(req_capacity);
	
	#undef req_capacity
}


void Kernel_Destroy_Mailbox(void)
{
	#define req_mb_id		Current_Process->request_args[0].val
		
	PtrList *i;
	MAILBOX_TYPE *mb;
		
	//Find the corresponding mailbox object in the mailbox list
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
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		return;
	}
	
	free_queue(&mb->mails);					//Destroy all pending mail 
	free(mb);
	ptrlist_remove(&MailboxList, i);
	--Mailbox_Count;
		
	
	
	#undef req_mb_id
}


void Kernel_Mailbox_Destroy_Mail(void)
{
	#define req_mail Current_Process->request_args[0].ptr
	
	MAIL* m = req_mail;
	
	/*Should we also check if the mail requested is still an unread mail sitting in a mailbox???*/
	
	if(!m)
	{
		#ifdef DEBUG
		printf("Kernel_Mailbox_Destroy_Mail: Attempted to destroy invalid mail\n");
		#endif
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		Current_Process->request_retval = 0;
	}
	
	free(m->ptr);
	m->ptr = NULL;
	m->size = 0;
	m->source = 0;
	
	
	Current_Process->request_retval = 1;
	
	#undef req_mail
}




/************************************************************************/
/*					Asynchronous Mailbox Operations		                */
/************************************************************************/

void Kernel_Mailbox_Check_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0].val
	
	MAILBOX_TYPE *mb = findMailboxByID(req_mb_id);
	
	if(!mb)
	{
		#ifdef DEBUG
		printf("Kernel_Mailbox_Check_Mail: The requested Mailbox %d was not found!\n", req_mb_id);
		#endif
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		Current_Process->request_retval = 0;
		return;
	}
	Current_Process->request_retval = mb->mails.count;
	
	#undef req_mb_id
}


void Kernel_Mailbox_Send_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0].val
	#define req_msg_ptr		Current_Process->request_args[1].ptr
	#define req_msg_size	Current_Process->request_args[2].val
	
	MAILBOX_TYPE *mb = findMailboxByID(req_mb_id);
	MAIL *m;
	
	if(!mb)
	{
		#ifdef DEBUG
		printf("Kernel_Mailbox_Send_Mail: The requested Mailbox %d was not found!\n", req_mb_id);
		#endif
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		Current_Process->request_retval = 0;
		return;
	}
	
	//Check if the mailbox still have free space left
	if(mb->mails.count >= mb->capacity)
	{
		#ifdef DEBUG
		printf("Mailbox Full, cannot send at this time...\n");
		#endif

		/*TODO: Maybe allow the sender to wait if the queue is full?*/

		Current_Process->request_retval = 0;
		return;
	}
	
	//Allocate a new MAIL object, and copy the specified message/data into it
	m = malloc(sizeof(MAIL));
	m->source = Current_Process->pid;
	m->size = req_msg_size;
	m->ptr = malloc(req_msg_size);
	
	if(!m->ptr)
	{
		printf("Malloc Failed...\n");
		kernel_raise_error(MALLOC_FAILED_ERR);
		Current_Process->request_retval = 0;
		return;
	}
	memcpy(m->ptr, req_msg_ptr, req_msg_size);
	
	//Add the new MAIL into the mailbox queue
	enqueue_ptr(&mb->mails, m);
	
	
	Current_Process->request_retval = mb->mails.count;
	
	#undef req_mb_id
	#undef req_msg_ptr
	#undef req_msg_size
}


void Kernel_Mailbox_Get_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0].val
	#define req_mail		Current_Process->request_args[1].ptr
	
	MAILBOX_TYPE *mb = findMailboxByID(req_mb_id);
	MAIL *m, *dest = req_mail;
	
	if(!mb)
	{
		#ifdef DEBUG
		printf("Kernel_Mailbox_Send_Mail: The requested Mailbox %d was not found!\n", req_mb_id);
		#endif
		
		req_mail = NULL;
		kernel_raise_error(OBJECT_NOT_FOUND_ERR);
		Current_Process->request_retval = 0;
		return;
	}
	
	if(mb->mails.count == 0)
	{
		#ifdef DEBUG
		printf("Mailbox is empty...\n");
		#endif
		
		dest->ptr = NULL;
		dest->size = 0;
		dest->source = 0;
		Current_Process->request_retval = 0;
		return;
	}
	
	m = dequeue_ptr(&mb->mails);
	dest->ptr = m->ptr;
	dest->size = m->size;
	dest->source = m->source;
	free(m);
	printf("Received %d bytes, from %d\n", dest->size, dest->source);
	
	Current_Process->request_retval = mb->mails.count;
	
	#undef req_mb_id
	#undef req_mail
}



/************************************************************************/
/*					Blocking Mailbox Operations		                */
/************************************************************************/

/*
void Kernel_Mailbox_Blocking_Send_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0].val
	#define req_msg_ptr		Current_Process->request_args[1].ptr
	#define req_msg_size	Current_Process->request_args[2].val
	//req_timeout is also used
	

	
	#undef req_mb_id
	#undef req_msg_ptr
	#undef req_msg_size
}


void Kernel_Mailbox_Blocking_Get_Mail(void)
{
	#define req_mb_id		Current_Process->request_args[0].val
	#define req_mail		Current_Process->request_args[1].ptr
	//req_timeout is also used
		

	
	#undef req_mb_id
	#undef req_mail
}
*/




