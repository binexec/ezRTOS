#include "my_malloc.h"

unsigned char* malloc_heap_start;		//Start of the dynamic memory heap
unsigned char* malloc_heap_end;			//Absolute end of the memory segment for the heap; cannot allocate further than this

//Note: malloc_heap_start > malloc_heap_end, since heap grows upwards

unsigned char* malloc_break;			//Also referred as "brk", the current end for the allocated heap	
size_t malloc_margin_size = 512;		//How many bytes to extend the heap at a time
Heap_FreeList *fl_head;					//Head of the first heap free list entry


int idx(unsigned char* addr)
{
	
	return 0;
}



/************************************************************************/
/*								MALLOC		  							*/
/************************************************************************/


int init_malloc(unsigned char* start, unsigned char* end)
{
	if(start < end)
	{
		printf("Heap starting address must be greater than end address!\n");
		return 0;
	}
	
	malloc_heap_start = start;
	malloc_heap_end = end;
	malloc_break = malloc_heap_start;	
	fl_head = NULL;
	
	printf("******Heap Start: %p, Heap End: %p\n\n", malloc_heap_start, malloc_heap_end);
	
	return 1;
}


static void create_new_freelist(unsigned char* address)
{
	
}


void* my_malloc(size_t len)
{
	Heap_FreeList *current_fl = NULL, *previous_fl = NULL;
	Heap_FreeList *next_smallest_fl = NULL, *next_smallest_fl_prev = NULL;
	Heap_FreeList *tail_fl = NULL, *tail_fl_prev = NULL;
	unsigned char* retaddr;
	
	int margin_blocks_needed;
	Heap_FreeList* newfl;
	
	unsigned char* new_break;
	
	
	/************************************************/
	/*			Step 0: Heap is uninitialized		*/
	/************************************************/
	
	if(malloc_break == malloc_heap_start && !fl_head)
	{
		//Allocate more memory to the heap by pushing back the malloc break
		margin_blocks_needed = (2*sizeof(Heap_FreeList) + len)/malloc_margin_size + 1;
		malloc_break -= margin_blocks_needed * malloc_margin_size;
		printf("Margin blocks needed: %d, size: %zu\n", margin_blocks_needed, margin_blocks_needed * malloc_margin_size);
		
		//Write a "used" freelist entry in the beginning of te heap
		newfl = (Heap_FreeList*)(malloc_heap_start - sizeof(Heap_FreeList));
		newfl->size = len;			
		newfl->next = NULL;
		retaddr = (unsigned char *)newfl - len;		//Grant memory for the request
		
		//Write a new freelist entry after the allocated memory
		newfl = (Heap_FreeList*)(retaddr - sizeof(Heap_FreeList));
		newfl->size = (unsigned char*)newfl - malloc_break;			
		newfl->next = NULL;
		fl_head = newfl;
		
		printf("Heap is currently unallocated!\n");
		printf("Allocated %zu bytes. Start: %p, End: %p, Brkval: %p\n", len, retaddr, malloc_heap_start, malloc_break);
		printf("New FL address: %p\n", newfl);
		printf("New FL Size: %zu\n", newfl->size);
		return retaddr;
	}
	
	
	/************************************************/
	/*			Step 1: Find an exact piece		    */
	/************************************************/
	
	
	/*Find the first Freelist large enough to fit the length requested*/
	for(current_fl = fl_head; current_fl; current_fl = current_fl->next)
	{
		//Found an exact piece
		if(current_fl->size == len)
		{
			//Disconnect the current piece from the fl chain
			if(previous_fl)
				previous_fl->next = current_fl->next;
			else
				fl_head = newfl;
			current_fl->next = NULL;
			
			retaddr = (unsigned char*)current_fl - len;
			printf("Found exact piece of size %zu at %p\n", len, retaddr);
			return retaddr;
		}
		
		//Record the smallest piece of memory available that can be splitted, if exact piece is not yet found available
		if(current_fl->size > (sizeof(Heap_FreeList) + len) && 
			(!next_smallest_fl || current_fl->size < next_smallest_fl->size))
		{
			next_smallest_fl = current_fl;
			next_smallest_fl_prev = previous_fl;
		}
		
		//Record the tail of the fl chain for step 3
		if(!current_fl->next)
		{
			tail_fl = current_fl;
			tail_fl_prev = previous_fl;
		}
		
		previous_fl = current_fl;
	}
	
	
	
	
	/************************************************/
	/*			Step 2: Split an larger piece	   	*/
	/************************************************/
	
	if(next_smallest_fl)
	{
		printf("Using a piece of size %zu at %p\n", next_smallest_fl->size, next_smallest_fl);
		
		//TODO: What happens if there's no space for a new FL?
	
		//Split a new fl for the excess part of the selected piece
		newfl = (Heap_FreeList*) (next_smallest_fl - len - sizeof(Heap_FreeList));
		newfl->size = next_smallest_fl->size - len - sizeof(Heap_FreeList);
		newfl->next = next_smallest_fl->next;
		
		printf("prevfl: %p, newfl: %p, diff: %ld, uint_diff:%zu\n", next_smallest_fl, newfl, next_smallest_fl-newfl, (unsigned long)next_smallest_fl-(unsigned long)newfl);
		
		//Disconnect the old piece from the fl chain, and update fl for the original piece
		if(next_smallest_fl_prev)
			next_smallest_fl_prev->next = newfl;
		else
			fl_head = newfl;
		next_smallest_fl->size = len;
		next_smallest_fl->next = NULL;	
		
		retaddr = (unsigned char*)next_smallest_fl - len;
		printf("Allocated %zu bytes. Start: %p\n", next_smallest_fl->size, retaddr);
		printf("Splitted into a new free piece of size %zu at %p\n", newfl->size, newfl);
		//printf("newfl: %p, newfl->size: %p, newfl->next: %p\n", newfl, &newfl->size, &newfl->next);
		return retaddr;
	}
	
	
	/************************************************/
	/*	Step 3: Allocate more heap space by adjusting brk	   	*/
	/************************************************/
	
	printf("Tail_FL address: %p\n", tail_fl);

	//Allocate more memory to the heap by pushing back the malloc break
	margin_blocks_needed = (sizeof(Heap_FreeList) + len - tail_fl->size)/malloc_margin_size + 1;
	printf("Additional blocks needed: %d, size: %zu\n", margin_blocks_needed, margin_blocks_needed * malloc_margin_size);
	printf("Remaining allocated free heap: %zu\n", tail_fl->size);
	
	//Push the malloc break back if we have space on the heap, otherwise fail
	new_break = malloc_break - margin_blocks_needed * malloc_margin_size;
	if(new_break < malloc_heap_end)
	{
		printf("Cannot continue. Allocation will exceed heap.\n");
		return NULL;
	}
	

	//Grant memory
	malloc_break = new_break;
	retaddr = (unsigned char *)tail_fl - len;
		
	//Write a new freelist entry after the allocated memory
	newfl = (Heap_FreeList*)(retaddr - sizeof(Heap_FreeList));
	newfl->size = (ptr_uint)newfl - (ptr_uint)malloc_break);			
	newfl->next = NULL;
	
	if(tail_fl_prev)
		tail_fl_prev->next = newfl;
	else
		fl_head = newfl;
	
	
	printf("Allocated %zu bytes. Start: %p, End: %p, Brkval: %p\n", len, retaddr, (unsigned char*)tail_fl, malloc_break);
	printf("New FL address: %p\n", newfl);
	//printf("New FL Size Expected: %zu\n", (unsigned char*)newfl - malloc_break);
	printf("New FL Size: %zu\n", newfl->size);
	return retaddr;
	
	//return NULL;

}



/************************************************************************/
/*								FREE		  							*/
/************************************************************************/

void my_free(void *p)
{
	
}


/************************************************************************/
/*								REALLOC		  							*/
/************************************************************************/

void* my_realloc(void *ptr, size_t len)
{
	return NULL;
}


