#include "Queue.h"


static Queue new_queue(void)
{
	Queue q;
	
	q.count = 0;
	q.head = 0;
	q.tail = 0;
	memset(q.queue, 0, sizeof(QElement)*QUEUE_LENGTH);
	
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

static int enqueue(Queue *q, QElement val)
{
	if(q->count >= QUEUE_LENGTH)
		return -1;
	
	if(q->count == 0)
	{
		q->queue[0] = val;
		q->head = 0;
		q->tail = 0;
		q->count = 1;
		return 0;
	}
		
	//Add the new item to the tail of the circular queue
	if(q->tail >= QUEUE_LENGTH-1)
		q->tail = 0;
	else
		q->tail++;
	
	q->queue[q->tail] = val;
	q->count++;
	
	return 0;
}

int enqueue_pid(Queue *q, PID p)
{
	QElement pval =  {.val = p};
	
	if(q->is_ptr_queue)
		return -1;
	
	return enqueue(q, pval);
}

int enqueue_ptr(Queue *q, void *p)
{
	QElement pval =  {.ptr = p};
	
	if(!q->is_ptr_queue)
		return -1;
	
	return enqueue(q, pval);
}


/************************************************************************/
/*								Dequeue                                 */
/************************************************************************/

static QElement dequeue(Queue *q)
{
	QElement retval;
	
	//Note: empty queue check is done by dequeue_pid/dequeue_ptr
	
	retval = q->queue[q->head];
	q->count--;
	
	//Set a new head for the queue
	if(q->head >= QUEUE_LENGTH-1)
		q->head = 0;
	else
		q->head++;
	
	//Reset head/tail to index 0 if the queue is now empty
	if(q->count <= 0)
	{
		q->head = 0;
		q->tail = 0;
	}
	
	return retval;
}

PID dequeue_pid(Queue *q)
{
	QElement retval;
	
	if(q->count <= 0)
		return 0;
	
	retval = dequeue(q);
	return retval.val;
}

void* dequeue_ptr(Queue *q)
{
	QElement retval;
	
	if(q->count <= 0)
		return NULL;
	
	retval = dequeue(q);
	return retval.ptr;
}



/************************************************************************/
/*								Peek       		                        */
/************************************************************************/

static QElement queue_peek(Queue *q)
{
	return q->queue[q->head];
}

PID queue_peek_pid(Queue *q)
{
	QElement retval;
	
	if(q->count <= 0)
		return 0;
	
	retval = queue_peek(q);
	return retval.val;
}

void* queue_peek_ptr(Queue *q)
{
	QElement retval;
	
	if(q->count <= 0)
		return NULL;
	
	retval = queue_peek(q);
	return retval.ptr;
}


/************************************************************************/
/*								Iterate       	                        */
/************************************************************************/

/*WARNING: Iterators are non-reentrant*/

static int iterate_queue(Queue *q, QElement *retval)
{
	static Queue *last_queue;
	static int last_idx;
	static int iterated;
	
	//Work like strtok, keep iterating last queue if q is NULL
	if(q != NULL)
	{
		//Don't iterate the new queue if it's empty
		if(q->count <= 0)
			return 0;
			
		last_queue = q;
		last_idx = q->head;
		iterated = 0;
	}
	else
	{
		//Don't return anything further if the entire queue has been iterated
		if(iterated == last_queue->count)
			return 0;
		
		//increment the index
		if(last_idx == QUEUE_LENGTH-1)
			last_idx = 0;
		else
			last_idx++;
	}
	++iterated;
	
	*retval = last_queue->queue[last_idx];
	return 1;
}

PID iterate_pid_queue(Queue *q)
{
	QElement retval;
	
	if(!iterate_queue(q, &retval))
		return 0;
		
	return retval.val;
}

void* iterate_ptr_queue(Queue *q)
{
	QElement retval;
	
	if(!iterate_queue(q, &retval))
		return NULL;
	
	return retval.ptr;
}



void print_queue(Queue *q)
{
	int i;
	
	if(q->is_ptr_queue)
	{
		printf("Cannot print pointer queue!\n");
		return;
	}
	
	printf("%d ", iterate_pid_queue(q));
	
	for(i=0; i<q->count-1; i++)
		printf("%d ", iterate_pid_queue(NULL));	
}
