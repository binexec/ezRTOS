#include "Queue.h"
#include <stdio.h>


void print_pid_queue(Queue *q)
{
	QElement *i;
	
	for(i = q->head; i; i = i->next)
	{
		printf("%d, ", i->intval);
	}
	printf("\n");
}


void test_pid_queue()
{
	Queue q = new_pid_queue();
	
	printf("Enqueue 0: \t%d\n", enqueue_pid(&q, 0));
	//printf("Head %d, Tail %d\n", q.head->intval, q.tail->intval);
	printf("Enqueue 1: \t%d\n", enqueue_pid(&q, 1));
	//printf("Head %d, Tail %d\n", q.head->intval, q.tail->intval);
	printf("Enqueue 2: \t%d\n", enqueue_pid(&q, 2));
	//printf("Head %d, Tail %d\n", q.head->intval, q.tail->intval);
	printf("Enqueue 3: \t%d\n", enqueue_pid(&q, 3));
	printf("Enqueue 4: \t%d\n", enqueue_pid(&q, 4));
	printf("Enqueue 5: \t%d\n", enqueue_pid(&q, 5));
	printf("Enqueue 6: \t%d\n", enqueue_pid(&q, 6));
	printf("Enqueue 7: \t%d\n", enqueue_pid(&q, 7));
	printf("Enqueue 8: \t%d\n", enqueue_pid(&q, 8));
	printf("Enqueue 9: \t%d\n", enqueue_pid(&q, 9));
			
	printf("\n\n");
	print_pid_queue(&q);
	printf("\n\n");
			
	printf("Dequeue 0: \t%d\n", dequeue_pid(&q));
	printf("Dequeue 1: \t%d\n", dequeue_pid(&q));
	printf("Dequeue 2: \t%d\n", dequeue_pid(&q));
			
	printf("\n\n");
	print_pid_queue(&q);
	printf("\n\n");
			
	printf("Enqueue 7: \t%d\n", enqueue_pid(&q, 7));
	printf("Enqueue 8: \t%d\n", enqueue_pid(&q, 8));
	printf("Enqueue 9: \t%d\n", enqueue_pid(&q, 9));
	printf("Enqueue 10: \t%d\n", enqueue_pid(&q, 10));
			
	printf("\n\n");
	print_pid_queue(&q);
	printf("\n\n");
}



/************************************************************************/
/*						Testing Pointer Queue   	   	                 */
/************************************************************************/

typedef struct {
	char *str;
	int leng;
}Msg;

Msg msgs[10] = {{"aaa", 3}, {"bbb", 3}, {"ccc", 3}, {"ddd", 3}, 
				{"eee", 3}, {"fff", 3}, {"ggg", 3}, {"hhh", 3},
				{"iii", 3}, {"jjj", 3}};
	

	
void print_msg_queue(Queue *q)
{
	QElement *i;
	Msg *current;
	
	for(i = q->head; i; i = i->next)
	{
		current = i->ptrval;
		printf("%d, %s\n", current->leng, current->str);
	}
	
}


void test_ptr_queue()
{
	Queue q = new_ptr_queue();
	Msg *temp;
	
	printf("Enqueue aaa: \t%d\n", enqueue_ptr(&q, &msgs[0]));
	printf("Enqueue bbb: \t%d\n", enqueue_ptr(&q, &msgs[1]));
	printf("Enqueue ccc: \t%d\n", enqueue_ptr(&q, &msgs[2]));
	printf("Enqueue ddd: \t%d\n", enqueue_ptr(&q, &msgs[3]));
	printf("Enqueue eee: \t%d\n", enqueue_ptr(&q, &msgs[4]));
	printf("Enqueue fff: \t%d\n", enqueue_ptr(&q, &msgs[5]));
	printf("Enqueue ggg: \t%d\n", enqueue_ptr(&q, &msgs[6]));
	printf("Enqueue hhh: \t%d\n", enqueue_ptr(&q, &msgs[7]));
	printf("Enqueue iii: \t%d\n", enqueue_ptr(&q, &msgs[8]));
	printf("Enqueue jjj: \t%d\n", enqueue_ptr(&q, &msgs[9]));
			
			
	printf("\n\n");
	print_msg_queue(&q);
	printf("\n\n");
	
	
	printf("Dequeue aaa: ");
	temp = dequeue_ptr(&q);
	printf("\t%s\n", temp->str);
	printf("Dequeue bbb: ");
	temp = dequeue_ptr(&q);
	printf("\t%s\n", temp->str);
	printf("Dequeue ccc: ");
	temp = dequeue_ptr(&q);
	printf("\t%s\n", temp->str);
	
			
	printf("\n\n");
	print_msg_queue(&q);
	printf("\n\n");
	
			
	printf("Enqueue ggg: \t%d\n", enqueue_ptr(&q, &msgs[6]));
	printf("Enqueue hhh: \t%d\n", enqueue_ptr(&q, &msgs[7]));
	printf("Enqueue iii: \t%d\n", enqueue_ptr(&q, &msgs[8]));
	printf("Enqueue jjj: \t%d\n", enqueue_ptr(&q, &msgs[9]));
			
			
	printf("\n\n");
	print_msg_queue(&q);
	printf("\n\n");
}

int main()
{
	//test_pid_queue();
	test_ptr_queue();
}