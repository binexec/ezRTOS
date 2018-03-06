#include <strings.h>
#include <stdio.h>
#include <stdint.h>

#ifndef NULL
#define NULL 0
#endif


/*
*	Represents a piece of free memory on the heap, forming a chain of freelist. 
*	This data structure is also used to mark an allocated piece of memory within the heap, 
*	but allocated memory do not form any chains/lists
*/
typedef struct heap_seg{
	
	size_t size;
	struct heap_seg *next;
	
}Heap_Seg;


typedef struct {
	
	unsigned char* malloc_heap_start;
	unsigned char* malloc_heap_end;
	unsigned char* malloc_break;
	Heap_Seg *freelist_head;
	
}Malloc_Param;



int init_malloc(unsigned char* start, unsigned char* end);
Malloc_Param save_malloc_param(void);
void load_malloc_param(Malloc_Param p);

void* my_malloc(size_t len);
void* my_calloc(size_t nitems, size_t size);
void my_free(void *p);
void* my_realloc(void *ptr, size_t len);
