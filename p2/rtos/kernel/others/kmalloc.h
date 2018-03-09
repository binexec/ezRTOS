#ifndef KMALLOC_H
#define KMALLOC_H

//#include <strings.h>
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
	
	unsigned char* kmalloc_heap_start;
	unsigned char* kmalloc_heap_end;
	unsigned char* kmalloc_break;
	Heap_Seg *freelist_head;
	
}kMalloc_Param;



int init_kmalloc(unsigned char* start, unsigned char* end);
kMalloc_Param save_kmalloc_param(void);
void load_kmalloc_param(kMalloc_Param p);

void* kmalloc(size_t len);
void* kcalloc(size_t nitems, size_t size);
void kfree(void *p);
void* krealloc(void *ptr, size_t len);


#endif /* KMALLOC_H */