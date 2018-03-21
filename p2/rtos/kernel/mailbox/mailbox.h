#ifndef MAILBOX_H_
#define MAILBOX_H_

#include "../kernel_shared.h"
#include "../others/Queue.h"

#define MAXMAILBOX					8


typedef struct MAIL{
	
	void* ptr;
	unsigned int size;
	PID source;
	
} MAIL;


typedef struct {

	MAILBOX id;
	unsigned int capacity;
	Queue mails;
	Queue send_queue;	
	Queue recv_queue;
	
} MAILBOX_TYPE;


/*Shared variables*/
extern volatile unsigned int Last_MailboxID;



void Mailbox_Reset(void);
MAILBOX_TYPE* findMailboxByID(MAILBOX mb);

void Kernel_Create_Mailbox(void);
MAILBOX Kernel_Create_Mailbox_Direct(unsigned int capacity);
void Kernel_Destroy_Mailbox(void);
void Kernel_Mailbox_Destroy_Mail(void);

void Kernel_Mailbox_Check(void);
void Kernel_Mailbox_Send(void);
void Kernel_Mailbox_Recv(void);









#endif /* MAILBOX_H_ */