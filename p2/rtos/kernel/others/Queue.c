#include "Queue.h"
#include <stdlib.h>		//Remove once kmalloc is used


static Queue new_queue(void)
{
	Queue q;
	
	q.head = NULL;
	q.tail = NULL;
	q.count = 0;
	
	return q;
}


Queue new_pid_queue(void)
{
	Queue q = new_queue();	
	q.is_ptr_queue = 0;
	
	return q;
}

Queue new_ptr_queue(void)
{
	Queue q = new_queue();	
	q.is_ptr_queue = 1;

	return q;
}

int get_queue_type(Queue *q)
{
	return q->is_ptr_queue;		//0 if int queue; 1 if pointer queue
}



/************************************************************************/
/*								Enqueue                                 */
/************************************************************************/

static void enqueue(Queue *q, QElement *qe)
{
	if(!q->head)
		q->head = qe;
	else
		q->tail->next = qe;
	
	q->tail = qe;
	q->count++;
}

int enqueue_pid(Queue *q, PID p)
{
	QElement *qe;
	
	if(q->is_ptr_queue)
		return -1;
		
	qe = malloc(sizeof(QElement));
	qe->intval = p;
	qe->next = NULL;
	enqueue(q, qe);
	
	return 1;
}

int enqueue_ptr(Queue *q, void *p)
{
	QElement *qe;
	
	if(!q->is_ptr_queue)
		return -1;
	
	qe = malloc(sizeof(QElement));
	qe->ptrval = p;
	qe->next = NULL;
	enqueue(q, qe);
	
	return 1;
}


/************************************************************************/
/*								Dequeue                                 */
/************************************************************************/

static QElement* dequeue(Queue *q)
{
	QElement *retval;
	
	if(!q->head || q->count <= 0)
		return NULL;
	
	//Pop the head 
	retval = q->head;
	q->head = q->head->next;
	--q->count;
	
	//If queue is empty after the pop
	if(!q->head)
	{
		q->tail = NULL;
		q->count = 0;
	}
	
	return retval;
}

PID dequeue_pid(Queue *q)
{
	QElement *qe;
	PID retval;
	
	qe = dequeue(q);
	if(!qe)
		return 0;
	
	retval = qe->intval;
	free(qe);
	
	return retval;
}

void* dequeue_ptr(Queue *q)
{
	QElement *qe;
	void* retval;
	
	qe = dequeue(q);
	if(!qe)
		return NULL;
	
	retval = qe->ptrval;
	free(qe);
	
	return retval;
}



/************************************************************************/
/*								Peek       		                        */
/************************************************************************/

static QElement* queue_peek(Queue *q)
{
	return q->head;
}

PID queue_peek_pid(Queue *q)
{
	QElement *retval = queue_peek(q);
	
	if(!retval || q->count <= 0)
		return 0;
	
	return retval->intval;
}

void* queue_peek_ptr(Queue *q)
{
	QElement *retval = queue_peek(q);
	
	if(!retval || q->count <= 0)
		return NULL;
	
	return retval->ptrval;
}


