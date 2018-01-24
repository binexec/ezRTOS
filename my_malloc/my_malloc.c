/*
Note: malloc_heap_start > malloc_heap_end, since heap grows upwards

Adding an offset to a memory address will make it go down, and subtracting will make it go up.
*/

#include "my_malloc.h"

typedef unsigned char uchar;

uchar* malloc_heap_start;						//Start of the dynamic memory heap
uchar* malloc_heap_end;							//Absolute end of the memory segment for the heap; cannot allocate further than this

uchar* malloc_break;							//Also referred as "brk", the current end for the allocated heap	
size_t malloc_margin_size = 512;				//How many bytes to extend the heap at a time
Heap_Seg *freelist_head;					//Head of the first heap free list entry


#define MAX_HEAP_SIZE	(size_t)(malloc_heap_start - malloc_heap_end)


/************************************************************************/
/*							INITIALIZATION		 						*/
/************************************************************************/

int init_malloc(uchar* start, uchar* end)
{
	if(start < end)
	{
		printf("Heap starting address must be greater than end address!\n");
		return 0;
	}
	
	malloc_heap_start 	= start;
	malloc_heap_end 	= end;
	malloc_break 		= malloc_heap_start;	
	freelist_head 		= NULL;
	
	printf("******Heap Start: %p, Heap End: %p\n\n", malloc_heap_start, malloc_heap_end);
	//printf("sizeof(Heap_Seg): %zu\n", sizeof(Heap_Seg));
	
	return 1;
}


Malloc_Param save_malloc_param(void)
{
	Malloc_Param p;
	
	p.malloc_heap_start 	= malloc_heap_start;
	p.malloc_heap_end 		= malloc_heap_end;
	p.malloc_break 			= malloc_break;
	p.malloc_margin_size 	= malloc_margin_size;
	p.freelist_head 		= freelist_head;
	
	return p;
}


void load_malloc_param(Malloc_Param p)
{
	malloc_heap_start 	= p.malloc_heap_start;
	malloc_heap_end 	= p.malloc_heap_end;
	malloc_break 		= p.malloc_break;
	malloc_margin_size 	= p.malloc_margin_size;
	freelist_head 		= p.freelist_head;
}


/************************************************************************/
/*								MALLOC		  							*/
/************************************************************************/

void* my_malloc(size_t len)
{
	
	Heap_Seg *current_piece = NULL, *previous_piece = NULL;
	Heap_Seg *exact_piece = NULL, *exact_piece_prev = NULL;
	Heap_Seg *next_smallest_piece = NULL, *next_smallest_piece_prev = NULL;
	Heap_Seg *freelist_tail = NULL, *freelist_tail_prev = NULL;
	
	uchar* retaddr = NULL;
	Heap_Seg *new_entry;
	
	int margin_blocks_needed;
	uchar* new_break = NULL;
	
	
	/*If stack has not yet been initialized, skip to step 3*/
	

	/************************************************/
	/*		Attempt 1: Find an exact piece		    */
	/************************************************/
	
	
	/*Find the first Freelist large enough to fit the length requested*/
	for(current_piece = freelist_head; current_piece; current_piece = current_piece->next, previous_piece = current_piece)
	{
		//Found an exact piece
		if(current_piece->size == len)
		{
			exact_piece = current_piece;
			exact_piece_prev = previous_piece;
			break;
		}
		
		//Record the smallest piece of memory available (of at least length "len"), if exact piece is not yet found
		if(current_piece->size > len + sizeof(Heap_Seg) && 
			(!next_smallest_piece || current_piece->size < next_smallest_piece->size))
		{
			next_smallest_piece = current_piece;
			next_smallest_piece_prev = previous_piece;
		}
		
		//Record the tail of the fl chain for step 3
		if(!current_piece->next)
		{
			freelist_tail = current_piece;
			freelist_tail_prev = previous_piece;
		}
	}

	
	//Grant the exact piece if available
	if(exact_piece)
	{
		retaddr = (uchar*)exact_piece + sizeof(Heap_Seg);
			
		//Disconnect the current piece from the fl chain
		//If the piece used have no previous free piece, that means this is also freelist_head
		exact_piece->next = NULL;
		
		if(exact_piece_prev)
			exact_piece_prev->next = exact_piece->next;
		else
			freelist_head = exact_piece->next;	

		printf("Found exact piece of size %zu at %p\n", len, retaddr);
		printf("Allocated %zu bytes. Start: %p\n", exact_piece->size, retaddr);
		return retaddr;
	}
	
	
	/************************************************/
	/*		Attempt 2: Split an larger piece	   	*/
	/************************************************/

	if(next_smallest_piece)
	{
		printf("Using a piece of size %zu at %p\n", next_smallest_piece->size, next_smallest_piece);
		
		//Grant memory
		retaddr = (uchar*)next_smallest_piece + sizeof(Heap_Seg);
		
		
		//Split a new fl entry for the excess free part of the selected piece.
		//Note that the new free piece is on the LEFT of the allocated split piece
		new_entry = (Heap_Seg*) (retaddr + len);
		new_entry->size = next_smallest_piece->size - len - sizeof(Heap_Seg);
		new_entry->next = next_smallest_piece->next;
		
		
		//Since next_smallest_piece is now the allocated portion, we'll disconnect it from the freelist
		next_smallest_piece->size = len;
		next_smallest_piece->next = NULL;

		if(next_smallest_piece_prev)
			next_smallest_piece_prev->next = new_entry;
		else
			freelist_head = new_entry;
		
		
		printf("Allocated %zu bytes. Start: %p\n", next_smallest_piece->size, retaddr);
		printf("Splitted into a new free piece at %p. Size: %zu, Next: %p\n", new_entry, new_entry->size, new_entry->next);
		return retaddr;
	}
	
	
	/************************************************/
	/*		Attempt 3: Allocate more heap space 	*/
	/************************************************/
	
	
	/*If heap is uninitialized or the allocated heap is full*/
	if(!freelist_head)
	{	
		//Calculate how many new blocks is needed
		margin_blocks_needed = (2*sizeof(Heap_Seg) + len)/malloc_margin_size + 1;
		
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
	if(freelist_tail)
	{
		printf("Tail_FL address: %p\n", freelist_tail);
		printf("Remaining allocated free heap: %zu\n", freelist_tail->size);
		
		//Calculate how many new blocks is needed
		margin_blocks_needed = (len - freelist_tail->size + sizeof(Heap_Seg))/malloc_margin_size + 1;
			
		//Calculate the return address for the request (assuming additional heap will be granted)
		retaddr = (uchar*)freelist_tail + sizeof(Heap_Seg);
		
		//Because the tail piece will be enlarged, we'll wipe it as the header itself will be part of the granted memory
		freelist_tail->size = 0;
		freelist_tail->next = NULL;
	}
	
	
	//Allocate additional heap space by pushing the break back
	new_break = malloc_break - margin_blocks_needed * malloc_margin_size;	
	printf("Margin blocks needed: %d, size: %zu\n", margin_blocks_needed, margin_blocks_needed * malloc_margin_size);
	
	if(new_break < malloc_heap_end)
	{
		printf("Cannot continue. Allocation will exceed heap.\n");
		return NULL;
	}	
	malloc_break = new_break;
		
	//Write a "used" allocation entry after the allocated memory
	new_entry = (Heap_Seg*)(retaddr - sizeof(Heap_Seg));
	new_entry->size = len;			
	new_entry->next = NULL;
	printf("Allocated %zu bytes at %p\n", new_entry->size, retaddr);
		
	//Write a new freelist entry at the malloc break
	new_entry = (Heap_Seg*)malloc_break;
	new_entry->size = ((uchar*)retaddr - sizeof(Heap_Seg)) - (malloc_break + sizeof(Heap_Seg));	
	new_entry->next = NULL;
	printf("New FL at: %p, size: %zu, Brkval: %p\n", new_entry, new_entry->size, malloc_break);
	
	//Update freelist chain
	if(freelist_tail_prev)
		freelist_tail_prev->next = new_entry;
	else
		freelist_head = new_entry;
	

	return retaddr;
}



/************************************************************************/
/*								FREE		  							*/
/************************************************************************/

void my_free(void *p)
{
	Heap_Seg *p_entry = p - sizeof(Heap_Seg);
	Heap_Seg *new_entry, *new_entry_prev = NULL;
	
	Heap_Seg *current_piece = NULL;
	Heap_Seg *closest_left = NULL, *closest_left_prev = NULL;
	Heap_Seg *closest_right = NULL;
	
	size_t break_reduced;
	
	/************************************************/
	/*				Step 0: Sanity Check			*/
	/************************************************/
	
	if(p == NULL)
		return;
	
	if((uchar*)p < malloc_break || (uchar*)p < malloc_heap_end || (uchar*)p > malloc_heap_start)
		return;
	
	if(p_entry->size > MAX_HEAP_SIZE || p_entry->next != NULL)
	{
		printf("p does not seem to have a valid allocation entry!\n");
		return;
	}
	
	printf("(FREEING %p) P_size: %zu, P_next: %p\n", p, p_entry->size, p_entry->next);
	
	
	/************************************************/
	/*		Step 1: Freeing the requested piece 	*/
	/************************************************/
	
	//Iterate through the current freelist and locate the closest free blocks to p
	for(current_piece = freelist_head; current_piece; current_piece = current_piece->next)
	{
		if(current_piece > p_entry)
		{
			closest_left_prev = closest_left;
			closest_left = current_piece;
		}
		else if(current_piece < p_entry)
		{
			closest_right = current_piece;
			break;
		}
		
		//There should never be an occurance where current_piece == p_entry. If it does happen, we'll just ignore it
	}
	
	//Write a new freelist entry at the beginning of the freed block, and destroy the old "used" allocation entry at the end of the piece
	new_entry = p_entry;
	if(!closest_left)
	{
		new_entry->next = freelist_head;
		freelist_head = new_entry;
	}
	else
	{
		new_entry->next = closest_left->next;
		closest_left->next = new_entry;
		new_entry_prev = closest_left;
	}
	
	if(closest_left)
		printf("Has Left at %p\n", closest_left);
	if(closest_right)
		printf("Has Right at %p\n", closest_right);
	printf("Current freelist head at %p\n", freelist_head);
	
	printf("New FL at %p, size: %zu, next: %p\n", (uchar*)new_entry, new_entry->size, new_entry->next);
	
	
	/************************************************/
	/*	Step 2: Merge with adjacent left piece	 	*/
	/************************************************/
	
	if(	closest_left && 
		(uchar*)new_entry + sizeof(Heap_Seg) + new_entry->size == (uchar*)closest_left)
	{
		printf("Found left adjacent piece at %p. Size: %zu, Next: %p\n", (uchar*)closest_left, closest_left->size, closest_left->next);
		
		new_entry->size += closest_left->size + sizeof(Heap_Seg);
		closest_left->size = 0;
		closest_left->next = 0;
		
		if(closest_left_prev)
		{
			closest_left_prev->next = new_entry;
			new_entry_prev = closest_left_prev;
		}
		else
			freelist_head = new_entry;
		
		printf("Merged! New Size: %zu, Next: %p\n", new_entry->size, new_entry->next);
	}
	
	
	/************************************************/
	/*	Step 3: Merge with adjacent right piece	 	*/
	/************************************************/
	
	if(	closest_right && 
		(uchar*)closest_right + sizeof(Heap_Seg) + closest_right->size == (uchar*)new_entry)
	{
		printf("Found right adjacent piece at %p. Size: %zu, Next: %p\n", (uchar*)closest_right, closest_right->size, closest_right->next);
		
		closest_right->size += new_entry->size + sizeof(Heap_Seg);
		new_entry->size = 0;
		new_entry->next = 0;
		new_entry = closest_right;
		
		if(new_entry_prev)
			new_entry_prev->next = closest_right;
		else
			freelist_head = new_entry;
		
		printf("Merged! New Size: %zu, Next: %p\n", new_entry->size, new_entry->next);
	}
	
	
	/************************************************/
	/*			Step 4: Reduce Malloc Break		 	*/
	/************************************************/
	
	//Only reduce the break if the last free piece is greater than one margin
	//Heap will only reduce/increase by exact margin sizes
	
	if((uchar*)new_entry == malloc_break && !new_entry->next)
	{
		//If the last free piece exceeds one margin
		if(new_entry->size > malloc_margin_size)
		{
			break_reduced = new_entry->size / malloc_margin_size;
			break_reduced *= malloc_margin_size;
			
			//Reduce the break, and write a new freelist entry at the new break
			malloc_break += break_reduced;
			new_entry = (Heap_Seg*)malloc_break;
			new_entry->size -= break_reduced;
			new_entry->next = NULL;
			
			//Update freelist
			if(new_entry_prev)
				new_entry_prev->next = new_entry;
			else
				freelist_head = new_entry;
			
			printf("Reduced break by %zu bytes! New Size: %zu, Next: %p\n", break_reduced, new_entry->size, new_entry->next);
		}
		
		//If the last free piece has the exact size as one margin
		else if(new_entry->size + sizeof(Heap_Seg) == malloc_margin_size)
		{
			
			malloc_break += malloc_margin_size;
			new_entry->size = 0;
			new_entry_prev->next = NULL;

			//Update freelist
			if(new_entry_prev)
				new_entry_prev->next = NULL;
			else
				freelist_head = NULL;
			
			printf("Reduced break by exactly 1 margin! New Tail at %p, Size: %zu, Next: %p\n", new_entry_prev, new_entry_prev->size, new_entry->next);
		}

	}
	
	
}


/************************************************************************/
/*								REALLOC		  							*/
/************************************************************************/

void* my_realloc(void *ptr, size_t len)
{
	return NULL;
}


