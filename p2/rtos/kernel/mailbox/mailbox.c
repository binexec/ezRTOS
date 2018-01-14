#include "mailbox.h"

volatile MAILBOX_TYPE Mailboxes[MAXMAILBOX];

volatile unsigned int Mailbox_Count;
volatile unsigned int Last_Mailbox_ID;


void reset_mailbox(void)
{
	Mailbox_Count = 0;
	Last_Mailbox_ID = 0;	
	memset(Mailboxes, 0, sizeof(MAILBOX_TYPE)*MAXMAILBOX);
}


MAILBOX Kernel_Create_Mailbox(void)
{
	return 0;
}

int Kernel_Mailbox_Check_Mail(MAILBOX mb)
{
	return 0;
}

int Kernel_Mailbox_Send_Mail(MAILBOX mb, MAIL m)
{
	return 0;
}

int Kernel_Mailbox_Get_Mail(MAILBOX mb, MAIL* received)
{
	return 0;
}

int Kernel_Mailbox_Wait_Mail(MAILBOX mb, MAIL* received, TICK timeout)
{
	return 0;
}
