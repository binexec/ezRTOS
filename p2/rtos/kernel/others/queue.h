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
} Queue;

//Queue new_queue(void);
void init_queue(Queue *q);

int enqueue(Queue *q, PID val);
PID dequeue(Queue *q);
PID peek(Queue *q);

void print_queue(Queue *q);



#endif /* QUEUE_H_ */