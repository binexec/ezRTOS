#ifndef EVENT_GROUP_H_
#define EVENT_GROUP_H_

#include "../kernel_internal.h"

#define EVENT_BITS		16

typedef struct {	
	EVENT_GROUP id;
	unsigned int events :EVENT_BITS;
} EVENT_GROUP_TYPE;


void Event_Group_Reset(void);
unsigned int Kernel_Create_Event_Group(void);
void Kernel_Event_Group_Set_Bits(EVENT_GROUP e, unsigned int bits_to_set);
void Kernel_Event_Group_Clear_Bits(EVENT_GROUP e, unsigned int bits_to_clear);
void Kernel_Event_Group_Wait_Bits(EVENT_GROUP e, unsigned int bits_to_wait, unsigned int wait_all_bits, TICK timeout);
unsigned int Kernel_Event_Group_Get_Bits(EVENT_GROUP e);


extern volatile unsigned int Event_Group_Count;
extern volatile unsigned int Last_Event_Group_ID;

#endif /* EVENT_GROUP_H_ */