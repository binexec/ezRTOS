#include "PtrList.h"
#include <stdio.h>

void main()
{
	PtrList list = {.ptr = NULL, .next = NULL};
	char str[4][11] = {"aaaaaaaaaa", "bbbbbbbbbb", "cccccccccc", "dddddddddd"};
	PtrList *list_elements[4];
	
	PtrList *temp;
	int i;
	
	
	printf("Test Adding:\n");
	for(i=0; i<4; i++)
	{
		list_elements[i] = ptrlist_addtail(&list, str[i]);
	}
	
	for(temp = &list; temp; temp = temp->next)
	{
		printf("%s\n", (char*)temp->ptr);
	}
	
	
	
	printf("Test Adding:\n");
	for(i=0; i<4; i++)
	{
		list_elements[i] = ptrlist_addtail(&list, str[i]);
	}
	
	for(temp = &list; temp; temp = temp->next)
	{
		printf("%s\n", (char*)temp->ptr);
	}
	
	

	printf("\nTest cnext:\n");
	temp = &list;
	for(i=0; i<12; i++)
	{
		printf("%s\n", (char*)temp->ptr);
		temp = ptrlist_cnext(&list, temp);
	}
	
	
	
	printf("\nDestroying List...\n");
	ptrlist_reset(&list);
	
	
}