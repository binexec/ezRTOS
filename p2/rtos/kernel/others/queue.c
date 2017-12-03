#include "queue.h"

/*
Queue new_queue(void)
{
	Queue q;
	
	q.count = 0;
	q.head = 0;
	q.tail = 0;
	memset(q.queue, 0, sizeof(PID)*QUEUE_LENGTH);
	
	return q;
}
*/

void init_queue(Queue *q)
{
	q->count = 0;
	q->head = 0;
	q->tail = 0;
	memset(q->queue, 0, sizeof(PID)*QUEUE_LENGTH);
}

int enqueue(Queue *q, PID val)
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

PID dequeue(Queue *q)
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

PID peek(Queue *q)
{
	return q->queue[q->head];
}

void print_queue(Queue *q)
{
	int i, j;
	
	if(q->count <= 0)
	{
		printf("*Empty queue*");
		return;
	}
	
	j = q->head;
	for(i=0; i<q->count; i++)
	{
		printf("%d ", q->queue[j]);
		
		//increment j
		if(j == QUEUE_LENGTH-1)
			j = 0;
		else
			j++;
	}
	
	printf("\n");
	
}