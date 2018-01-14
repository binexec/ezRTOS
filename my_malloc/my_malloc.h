#include <strings.h>
#include <stdio.h>
#include <stdint.h>

#ifndef NULL
#define NULL 0
#endif

typedef struct heap_freelist{
	
	size_t size;
	struct heap_freelist *next;
	
}Heap_FreeList;


int init_malloc(unsigned char* start, unsigned char* end);

void* my_malloc(size_t len);
void my_free(void *p);
void* my_realloc(void *ptr, size_t len);