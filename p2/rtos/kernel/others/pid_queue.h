#ifndef QUEUE_H_
#define QUEUE_H_

#include "../../os.h"
#include <string.h>

#define QUEUE_LENGTH	MAXTHREAD		//Maximum length of each queue

typedef struct {	
	PID queue[QUEUE_LENGTH];
	unsigned int count;
	unsigned int head;
	unsigned int tail;	
} PID_Queue;

PID_Queue new_queue(void);

int enqueue(PID_Queue *q, PID val);
PID dequeue(PID_Queue *q);
PID queue_peek(PID_Queue *q);
PID iterate_queue(PID_Queue *q);
void print_queue(PID_Queue *q);



#endif /* QUEUE_H_ */