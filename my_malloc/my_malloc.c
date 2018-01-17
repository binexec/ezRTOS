#include "my_malloc.h"

typedef unsigned char uchar;

uchar* malloc_heap_start;		//Start of the dynamic memory heap
uchar* malloc_heap_end;			//Absolute end of the memory segment for the heap; cannot allocate further than this

//Note: malloc_heap_start > malloc_heap_end, since heap grows upwards

uchar* malloc_break;			//Also referred as "brk", the current end for the allocated heap	
size_t malloc_margin_size = 512;		//How many bytes to extend the heap at a time
Heap_FreeList *fl_head;					//Head of the first heap free list entry


#define MAX_HEAP_SIZE	(size_t)(malloc_heap_start - malloc_heap_end)


/************************************************************************/
/*								MALLOC		  							*/
/************************************************************************/


int init_malloc(uchar* start, uchar* end)
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


void* my_malloc(size_t len)
{
	Heap_FreeList *current_fl = NULL, *previous_fl = NULL;
	Heap_FreeList *exact_fl = NULL, *exact_fl_prev = NULL;
	Heap_FreeList *next_smallest_fl = NULL, *next_smallest_fl_prev = NULL;
	Heap_FreeList *tail_fl = NULL, *tail_fl_prev = NULL;
	uchar* retaddr = NULL;
	
	int margin_blocks_needed;
	Heap_FreeList* newfl;
	size_t orig_piece_size;
	Heap_FreeList* orig_piece_next;
	
	uchar* new_break;
	
	
	/*If stack has not yet been initialized, skip to step 3*/
	

	/************************************************/
	/*			Step 1: Find an exact piece		    */
	/************************************************/
	
	
	/*Find the first Freelist large enough to fit the length requested*/
	for(current_fl = fl_head; current_fl; current_fl = current_fl->next, previous_fl = current_fl)
	{
		//Found an exact piece
		if(current_fl->size == len)
		{
			exact_fl = current_fl;
			exact_fl_prev = previous_fl;
			break;
		}
		
		//Record the smallest piece of memory available (of at least length len), if exact piece is not yet found available
		if(current_fl->size > len + sizeof(Heap_FreeList) && 
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
	}

	
	//Grant the exact piece if available
	if(exact_fl)
	{
		retaddr = (uchar*)exact_fl + sizeof(Heap_FreeList) - len;
		
		//Write a "used" allocation entry after the allocated memory
		newfl = (Heap_FreeList*)(retaddr - sizeof(Heap_FreeList));
		newfl->size = len;			
		newfl->next = NULL;
			
		//Disconnect the current piece from the fl chain
		//If the piece used have no previous free piece, that means this is also fl_head
		if(exact_fl_prev)
			exact_fl_prev->next = exact_fl->next;
		else
			fl_head = exact_fl->next;	

		printf("Found exact piece of size %zu at %p\n", len, retaddr);
		printf("Allocated %zu bytes. Start: %p\n", newfl->size, retaddr);
		return retaddr;
	}
	
	
	
	/************************************************/
	/*			Step 2: Split an larger piece	   	*/
	/************************************************/
	
	if(next_smallest_fl)
	{
		printf("Using a piece of size %zu at %p\n", next_smallest_fl->size, next_smallest_fl);
		
		//Grant memory. Note the original fl entry at the beginning is now consumed as part of the granted memory
		retaddr = (uchar*)next_smallest_fl + sizeof(Heap_FreeList) - len;
		orig_piece_size = next_smallest_fl->size;
		orig_piece_next = next_smallest_fl->next;	
		
		//Write an allocation entry (just another name for fl) at the end of the chunk to be granted
		newfl = (Heap_FreeList*) (retaddr - sizeof(Heap_FreeList));
		newfl->size = len;
		newfl->next = NULL;
		printf("Allocated %zu bytes. Start: %p\n", newfl->size, retaddr);
		
		//Split a new fl for the excess free part of the selected piece. As usual, the fl is placed at the start of the free piece
		newfl = (Heap_FreeList*) ((uchar*)newfl - sizeof(Heap_FreeList));
		newfl->size = orig_piece_size - len - sizeof(Heap_FreeList);
		newfl->next = orig_piece_next;
		printf("Splitted into a new free piece of size %zu at %p\n", newfl->size, newfl);

		
		//Disconnect the old piece from the fl chain, and update fl for the original piece
		if(next_smallest_fl_prev)
			next_smallest_fl_prev->next = newfl;
		else
			fl_head = newfl;
		
		//Because the old fl in the beginning will now be part of the granted memory, it's better to wipe it
		next_smallest_fl->size = 0;
		next_smallest_fl->next = NULL;
		
		return retaddr;
	}
	
	
	/************************************************/
	/*	Step 3: Allocate more heap space by adjusting brk	   	*/
	/************************************************/
	
	
	/*If heap is uninitialized or the allocated heap is full*/
	if(!fl_head)
	{	
		//Calculate new break location
		margin_blocks_needed = (2*sizeof(Heap_FreeList) + len)/malloc_margin_size + 1;
		new_break = malloc_break - margin_blocks_needed * malloc_margin_size;		
		
		//Calculate the return address for the request. Is heap uninitialized?
		if(malloc_break == malloc_heap_start)
		{
			printf("Heap is currently unallocated!\n");	
			retaddr = malloc_heap_start - len;	
		}
		else
		{
			printf("Allocated heap is full!\n");	
			retaddr = malloc_break - len;	
		}
	}
	
	
	/*The tailing piece of the allocated piece is not large enough*/
	if(tail_fl)
	{
		//Calculate new break location
		margin_blocks_needed = (sizeof(Heap_FreeList) + len - tail_fl->size)/malloc_margin_size + 1;
		new_break = malloc_break - margin_blocks_needed * malloc_margin_size;
			
		//Calculate the return address for the request
		retaddr = (uchar *)tail_fl + sizeof(Heap_FreeList) - len;
		
		//Because the old fl in the beginning will now be part of the granted memory, it's better to wipe it
		tail_fl->size = 0;
		tail_fl->next = NULL;
		
		printf("Tail_FL address: %p\n", tail_fl);
		printf("Remaining allocated free heap: %zu\n", tail_fl->size);
	}
	

	printf("Margin blocks needed: %d, size: %zu\n", margin_blocks_needed, margin_blocks_needed * malloc_margin_size);
	
	//Push the break back
	if(new_break < malloc_heap_end)
	{
		printf("Cannot continue. Allocation will exceed heap.\n");
		return NULL;
	}	
	malloc_break = new_break;
		
	//Write a "used" allocation entry after the allocated memory
	newfl = (Heap_FreeList*)(retaddr - sizeof(Heap_FreeList));
	newfl->size = len;			
	newfl->next = NULL;
		
	//Write a new freelist entry after the allocated memory
	newfl = (Heap_FreeList*)((uchar*)newfl - sizeof(Heap_FreeList));
	newfl->size = (uchar*)newfl - malloc_break;			
	newfl->next = NULL;
	
	//Update freelist
	if(tail_fl && tail_fl_prev)
		tail_fl_prev->next = newfl;
	else
		fl_head = newfl;
	
	printf("Allocated %zu bytes at %p, Brkval: %p\n", newfl->size, retaddr, malloc_break);
	printf("New FL address: %p\n", newfl);
	printf("New FL Size: %zu\n", newfl->size);
	return retaddr;

}



/************************************************************************/
/*								FREE		  							*/
/************************************************************************/

void my_free(void *p)
{
	Heap_FreeList *p_fl = p - sizeof(Heap_FreeList);
	Heap_FreeList *p_newfl;
	
	Heap_FreeList *current_fl = NULL;
	Heap_FreeList *closet_fl = NULL;

	
	if(p == NULL)
		return;
	
	if((uchar*)p < malloc_break || (uchar*)p < malloc_heap_end || (uchar*)p > malloc_heap_start)
		return;
	
	if(p_fl->size > MAX_HEAP_SIZE || p_fl->next != NULL)
	{
		printf("p does not seem to be a valid, allocated freelist!\n");
		return;
	}
	
	printf("(FREEING %p) P_size: %zu, P_next: %p\n", p, p_fl->size, p_fl->next);
	
	/*If there is no other freelist entries*/
	if(!fl_head)
	{
		//If this piece was allocated to the break, lower the break
		if(p + p_fl->size == malloc_break)
			malloc_break = p + sizeof(Heap_FreeList);
		
		//If this piece is somewhere in the middle, then just set it as the fl head
		else
			fl_head = p_fl;	
	}
	
	
	/*Iterate through the current freelist and locate the closest free block to p*/
	
	p_newfl = p + p_fl->size - sizeof(Heap_FreeList);
	
	for(current_fl = fl_head; current_fl; current_fl = current_fl->next)
	{
		if(current_fl > p_newfl)
			closet_fl = current_fl;
		else
			break;
	}
	
	//Write a new fl entry at the beginning of the freed block
	if(closet_fl == NULL)
	{
		p_newfl->next = NULL;
		fl_head = p_newfl;
		printf("No predecessors\n");
	}
	else
	{
		p_newfl->next = closet_fl->next;
		closet_fl->next = p_newfl;
	}
	p_newfl->size = p_fl->size;
	
	
	p_fl->size = 0;
	
	printf("New FL at %p, size: %zu, next: %p\n", (uchar*)p_newfl, p_newfl->size, p_newfl->next);
	
}


/************************************************************************/
/*								REALLOC		  							*/
/************************************************************************/

void* my_realloc(void *ptr, size_t len)
{
	return NULL;
}


