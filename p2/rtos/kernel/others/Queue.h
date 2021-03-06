#ifndef QUEUE_H_
#define QUEUE_H_

#include "../../os.h"


/*Individual elements of a queue, allowing either a pointer or an int value*/
typedef struct queue_element{	
	union 
	{	
		unsigned int intval;
		void* ptrval;
	};
	
	struct queue_element *next;
	
} QElement;



/*Queue data structure*/
typedef struct {
		
	QElement *head;
	QElement *tail;
	unsigned int count;
	
	//If queue of pointer is used
	unsigned int is_ptr_queue :1;
		
} Queue;



Queue new_int_queue(void);
Queue new_ptr_queue(void);
int get_queue_type(Queue *q);
void free_queue(Queue *q);

int enqueue_int(Queue *q, PID p);
int enqueue_ptr(Queue *q, void *p);

PID dequeue_int(Queue *q);
void* dequeue_ptr(Queue *q);

PID queue_peek_int(Queue *q);
void* queue_peek_ptr(Queue *q);

PID iterate_int_queue(Queue *q);
void* iterate_ptr_queue(Queue *q);



void print_queue(Queue *q);



#endif /* QUEUE_H_ */