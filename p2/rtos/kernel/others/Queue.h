#ifndef QUEUE_H_
#define QUEUE_H_

#include "../../os.h"
#include <string.h>
#include <stdio.h>

#define QUEUE_LENGTH	MAXTHREAD		//Maximum length of each queue. Maybe allow each queue to have a different length?


/*Individual elements of a queue, allowing either a pointer or an int value*/
typedef union{
	
	unsigned int val;
	void* ptr;
	
} QElement;


/*Queue data structure*/
typedef struct {
		
	QElement queue[QUEUE_LENGTH];	//Note that a queue must contain only pointers, or only PIDs
	unsigned int count;
	unsigned int head;
	unsigned int tail;	
	
	//If queue of pointer is used
	unsigned int is_ptr_queue :1;
		
} Queue;



Queue new_pid_queue(void);
Queue new_ptr_queue(void);
int get_queue_type(Queue *q);

int enqueue_pid(Queue *q, PID p);
int enqueue_ptr(Queue *q, void *p);

PID dequeue_pid(Queue *q);
void* dequeue_ptr(Queue *q);

PID queue_peek_pid(Queue *q);
void* queue_peek_ptr(Queue *q);

PID iterate_pid_queue(Queue *q);
void* iterate_ptr_queue(Queue *q);



void print_queue(Queue *q);



#endif /* QUEUE_H_ */