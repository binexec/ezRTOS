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
	
} MAILBOX_TYPE;


/*Shared variables*/
extern volatile unsigned int Last_MailboxID;



void reset_mailbox(void);
MAILBOX_TYPE* findMailboxByID(MAILBOX mb);

void Kernel_Create_Mailbox(void);
MAILBOX Kernel_Create_Mailbox_Direct(unsigned int capacity);
void Kernel_Destroy_Mailbox(void);

int Kernel_Mailbox_Check_Mail(void);
int Kernel_Mailbox_Send_Mail(void);
int Kernel_Mailbox_Get_Mail(void);
int Kernel_Mailbox_Wait_Mail(void);







#endif /* MAILBOX_H_ */