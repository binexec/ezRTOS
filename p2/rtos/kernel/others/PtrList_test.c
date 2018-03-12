#include "PtrList.h"
#include <stdio.h>

#define N 5

void main()
{
	PtrList list = {.ptr = NULL, .next = NULL};
	char str[N][11] = {"aaaaaaaaaa", "bbbbbbbbbb", "cccccccccc", "dddddddddd", "eeeeeeeeee"};
	PtrList *list_elements[N];
	
	PtrList *temp;
	int i;
	
	
	printf("***Test Adding:\n");
	for(i=0; i<N; i++)
		list_elements[i] = ptrlist_addtail(&list, str[i]);
	
	for(temp = &list; temp; temp = temp->next)
		printf("%s\n", (char*)temp->ptr);
	
	printf("\n");
	
	
	
	printf("***Test Removal:\n");
	printf("Removing tail...\n");
	ptrlist_remove(&list, ptrlist_findtail(&list));
	for(temp = &list; temp; temp = temp->next)
		printf("%s\n", (char*)temp->ptr);
	printf("\n");
	
	printf("Removing mid (using find)...\n");
	ptrlist_remove(&list, ptrlist_find(&list, str[2]));
	for(temp = &list; temp; temp = temp->next)
		printf("%s\n", (char*)temp->ptr);
	printf("\n");
	
	printf("Removing head...\n");
	ptrlist_remove(&list, list_elements[0]);
	for(temp = &list; temp; temp = temp->next)
		printf("%s\n", (char*)temp->ptr);
	printf("\n");
	
	

	printf("***Test circular iterating:\n");
	temp = &list;
	for(i=0; i<N*2; i++)
	{
		printf("%s\n", (char*)temp->ptr);
		temp = ptrlist_cnext(&list, temp);
	}
	printf("\n");
	
	
	
	printf("***Destroying List...\n");
	ptrlist_destroy(&list);
	
	
}