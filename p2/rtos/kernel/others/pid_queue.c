#include "pid_queue.h"


PID_Queue new_queue(void)
{
	PID_Queue q;
	
	q.count = 0;
	q.head = 0;
	q.tail = 0;
	memset(q.queue, 0, sizeof(PID)*QUEUE_LENGTH);
	
	return q;
}

int enqueue(PID_Queue *q, PID val)
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

PID dequeue(PID_Queue *q)
{
	int val;
	
	if(q->count <= 0)
		return 0;
		
	//Retreive the head of the queue and increment the head
	val = q->queue[q->head];
	if(q->head >= QUEUE_LENGTH-1)
		q->head = 0;
	else
		q->head++;
		
	q->count--;
	
	//Reset head/tail to index 0 if the queue is now empty
	if(q->count <= 0)
	{
		q->head = 0;
		q->tail = 0;
	}
	
	return val;
}

PID queue_peek(PID_Queue *q)
{
	return q->queue[q->head];
}


PID iterate_queue(PID_Queue *q)
{
	static PID_Queue *last_queue;
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
	printf("%d ", last_queue->queue[last_idx]);
	return last_queue->queue[last_idx];
}


void print_queue(PID_Queue *q)
{
	int i;
	
	iterate_queue(q);
	
	for(i=0; i<q->count-1; i++)
		iterate_queue(NULL);
}
