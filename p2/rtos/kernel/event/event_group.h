#ifndef EVENT_GROUP_H_
#define EVENT_GROUP_H_

#include "../kernel_internal.h"

#define MAX_EVENT_BITS		16				//Maximum number of events represented within a event group

typedef struct {	
	
	EVENT_GROUP id;
	unsigned int events :MAX_EVENT_BITS;
	
} EVENT_GROUP_TYPE;


void Event_Group_Reset(void);
unsigned int Kernel_Create_Event_Group(void);
void Kernel_Event_Group_Set_Bits(void);
void Kernel_Event_Group_Clear_Bits(void);
void Kernel_Event_Group_Wait_Bits(void);
unsigned int Kernel_Event_Group_Get_Bits(void);


extern volatile unsigned int Event_Group_Count;
extern volatile unsigned int Last_Event_Group_ID;

#endif /* EVENT_GROUP_H_ */