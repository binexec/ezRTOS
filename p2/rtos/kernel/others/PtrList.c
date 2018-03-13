/*
This is a basic link list structure. To save CPU time at the cost of memory, consider implementing a doubly link list instead
*/

#include "PtrList.h"
#include <stdlib.h>
#include <stdio.h>



//Circular next
PtrList* ptrlist_cnext(PtrList *head, PtrList *current)
{
	return (current->next == NULL)? head:current->next;
}

PtrList* ptrlist_findtail(PtrList *head)
{
	if(!head)
	return NULL;
	
	for(;head->next; head = head->next);
	return head;
}

//Create a new list entry for ptr, and link it after the tail
PtrList* ptrlist_add(PtrList *head, void* ptr)
{
	PtrList* e;
	
	//If list is empty
	if(!head->ptr && !head->next)
	{
		head->ptr = ptr;
		return head;
	}

	//Create a new entry at the tail
	e = malloc(sizeof(PtrList));
	e->ptr = ptr;
	e->next = NULL;
	
	head = ptrlist_findtail(head);
	head->next = e;
	
	return e;
}

PtrList* ptrlist_find(PtrList *head, void* ptr)
{
	for(; head && head->ptr != ptr; head = head->next);
	
	//Will return NULL if not found
	return head;
}

int ptrlist_remove(PtrList *head, PtrList *to_remove)
{
	PtrList* temp;
	
	if(head == to_remove)
	{
		temp = head->next;
		
		//Simply make the head empty if it has no next
		if(!temp)
		{
			head->ptr = NULL;
			head->next = NULL;
			return 1;
		}
		
		//Copy next to the current head, and free its original entry
		head->ptr = temp->ptr;
		head->next = temp->next;
		free(temp);
		return 1;
	}
	
	//Iterate to the element before to_remove
	for(; head && head->next != to_remove; head = head->next);
	
	//to_remove is not in this list
	if(!head)
	return 0;
	
	head->next = to_remove->next;
	free(to_remove);
	
	return 1;
}


void ptrlist_destroy(PtrList *head)
{
	PtrList* i = head->next;
	
	//Clear the head first
	head->ptr = NULL;
	head->next = NULL;
	
	//Iterate through the rest of the list and free all remaining elements
	for(; !i; i = i->next)
	{
		printf("Freed!\n");
		free(i);
	}
}