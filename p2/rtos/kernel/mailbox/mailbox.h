#ifndef MAILBOX_H_
#define MAILBOX_H_

#include "../kernel_shared.h"
#include "../others/Queue.h"


#define MAXMAILBOX					8


typedef struct {
	
	void* ptr;
	unsigned int size;
	PID source;
	
} MAIL;

typedef struct {

	MAILBOX id;
	Queue value;	
	
} MAILBOX_TYPE;


void reset_mailbox(void);
MAILBOX Kernel_Create_Mailbox(void);
int Kernel_Mailbox_Check_Mail(MAILBOX mb);
int Kernel_Mailbox_Send_Mail(MAILBOX mb, MAIL m);
int Kernel_Mailbox_Get_Mail(MAILBOX mb, MAIL* received);
int Kernel_Mailbox_Wait_Mail(MAILBOX mb, MAIL* received, TICK timeout);




#endif /* MAILBOX_H_ */