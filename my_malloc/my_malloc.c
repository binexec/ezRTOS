#include "my_malloc.h"

typedef unsigned char uchar;

uchar* malloc_heap_start;						//Start of the dynamic memory heap
uchar* malloc_heap_end;							//Absolute end of the memory segment for the heap; cannot allocate further than this

//Note: malloc_heap_start > malloc_heap_end, since heap grows upwards

uchar* malloc_break;							//Also referred as "brk", the current end for the allocated heap	
size_t malloc_margin_size = 512;				//How many bytes to extend the heap at a time
Heap_FreeList *freelist_head;					//Head of the first heap free list entry


#define MAX_HEAP_SIZE	(size_t)(malloc_heap_start - malloc_heap_end)


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
	freelist_head = NULL;
	
	printf("******Heap Start: %p, Heap End: %p\n\n", malloc_heap_start, malloc_heap_end);
	//printf("sizeof(Heap_FreeList): %zu\n", sizeof(Heap_FreeList));
	
	return 1;
}


/************************************************************************/
/*								MALLOC		  							*/
/************************************************************************/

void* my_malloc(size_t len)
{
	
	Heap_FreeList *current_piece = NULL, *previous_piece = NULL;
	Heap_FreeList *exact_piece = NULL, *exact_piece_prev = NULL;
	Heap_FreeList *next_smallest_piece = NULL, *next_smallest_piece_prev = NULL;
	Heap_FreeList *freelist_tail = NULL, *freelist_tail_prev = NULL;
	
	Heap_FreeList* new_entry;
	int margin_blocks_needed;
	size_t orig_piece_size;
	Heap_FreeList* orig_piece_next;
	
	uchar* retaddr = NULL;
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
		if(current_piece->size > len + sizeof(Heap_FreeList) && 
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
		retaddr = (uchar*)exact_piece + sizeof(Heap_FreeList) - len;
		
		//Write a "used" allocation entry (just another name for fl) after the allocated memory. This is needed for free() later
		new_entry = (Heap_FreeList*)(retaddr - sizeof(Heap_FreeList));
		new_entry->size = len;			
		new_entry->next = NULL;
			
		//Disconnect the current piece from the fl chain
		//If the piece used have no previous free piece, that means this is also freelist_head
		if(exact_piece_prev)
			exact_piece_prev->next = exact_piece->next;
		else
			freelist_head = exact_piece->next;	

		printf("Found exact piece of size %zu at %p\n", len, retaddr);
		printf("Allocated %zu bytes. Start: %p\n", new_entry->size, retaddr);
		return retaddr;
	}
	
	
	/************************************************/
	/*		Attempt 2: Split an larger piece	   	*/
	/************************************************/

	if(next_smallest_piece)
	{
		printf("Using a piece of size %zu at %p\n", next_smallest_piece->size, next_smallest_piece);
		
		//Grant memory. Note the original fl entry at the beginning is now consumed as part of the granted memory
		retaddr = (uchar*)next_smallest_piece + sizeof(Heap_FreeList) - len;
		orig_piece_size = next_smallest_piece->size;
		orig_piece_next = next_smallest_piece->next;	
		
		//Write an "used" allocation entry at the end of the chunk to be granted
		new_entry = (Heap_FreeList*) (retaddr - sizeof(Heap_FreeList));
		new_entry->size = len;
		new_entry->next = NULL;
		printf("Allocated %zu bytes. Start: %p\n", new_entry->size, retaddr);
		
		//Split a new fl entry for the excess free part of the selected piece. As usual, the fl is placed at the start of the free piece
		new_entry = (Heap_FreeList*) ((uchar*)new_entry - sizeof(Heap_FreeList));
		new_entry->size = orig_piece_size - len - sizeof(Heap_FreeList);
		new_entry->next = orig_piece_next;
		printf("Splitted into a new free piece of size %zu at %p\n", new_entry->size, new_entry);

		//Disconnect the old piece from the fl chain, and update fl for the original piece
		if(next_smallest_piece_prev)
			next_smallest_piece_prev->next = new_entry;
		else
			freelist_head = new_entry;
		
		//Because the old fl in the beginning will now be part of the granted memory, it's better to wipe it
		next_smallest_piece->size = 0;
		next_smallest_piece->next = NULL;
		
		return retaddr;
	}
	
	
	/************************************************/
	/*		Attempt 3: Allocate more heap space 	*/
	/************************************************/
	
	
	/*If heap is uninitialized or the allocated heap is full*/
	if(!freelist_head)
	{	
		//Calculate how many new blocks is needed
		margin_blocks_needed = (2*sizeof(Heap_FreeList) + len)/malloc_margin_size + 1;
		/*margin_blocks_needed = sizeof(Heap_FreeList) + len;
		if(margin_blocks_needed == malloc_margin_size)
			malloc_margin_size = 1;
		else
			margin_blocks_needed = margin_blocks_needed/malloc_margin_size + 1;*/
		
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
		//Calculate how many new blocks is needed
		margin_blocks_needed = (sizeof(Heap_FreeList) + len - freelist_tail->size)/malloc_margin_size + 1;
		/*margin_blocks_needed = sizeof(Heap_FreeList) + len - freelist_tail->size;
		if(margin_blocks_needed == malloc_margin_size)
			malloc_margin_size = 1;
		else
			margin_blocks_needed = margin_blocks_needed/malloc_margin_size + 1;*/
			
		//Calculate the return address for the request
		retaddr = (uchar *)freelist_tail + sizeof(Heap_FreeList) - len;
		
		//Because the old fl in the beginning will now be part of the granted memory, it's better to wipe it
		freelist_tail->size = 0;
		freelist_tail->next = NULL;
		
		printf("Tail_FL address: %p\n", freelist_tail);
		printf("Remaining allocated free heap: %zu\n", freelist_tail->size);
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
	new_entry = (Heap_FreeList*)(retaddr - sizeof(Heap_FreeList));
	new_entry->size = len;			
	new_entry->next = NULL;
		
	//Write a new freelist entry after the allocated memory
	new_entry = (Heap_FreeList*)((uchar*)new_entry - sizeof(Heap_FreeList));
	new_entry->size = (uchar*)new_entry - malloc_break;			
	new_entry->next = NULL;
	
	//Update freelist chain
	if(freelist_tail_prev)
		freelist_tail_prev->next = new_entry;
	else
		freelist_head = new_entry;
	
	printf("Allocated %zu bytes at %p, Brkval: %p\n", new_entry->size, retaddr, malloc_break);
	printf("New FL address: %p\n", new_entry);
	printf("New FL Size: %zu\n", new_entry->size);
	return retaddr;
}



/************************************************************************/
/*								FREE		  							*/
/************************************************************************/

void my_free(void *p)
{
	Heap_FreeList *p_entry = p - sizeof(Heap_FreeList);
	Heap_FreeList *new_entry, *new_entry_prev = NULL;
	
	Heap_FreeList *current_piece = NULL;
	Heap_FreeList *closest_left = NULL, *closest_left_prev = NULL;
	Heap_FreeList *closest_right = NULL;
	
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
	new_entry = p + p_entry->size - sizeof(Heap_FreeList);
	for(current_piece = freelist_head; current_piece; current_piece = current_piece->next)
	{
		if(current_piece > new_entry)
		{
			closest_left_prev = closest_left;
			closest_left = current_piece;
		}
		else if(current_piece < new_entry)
		{
			closest_right = current_piece;
			break;
		}
		
		//There should never be an occurance where current_piece == new_entry. If it does happen, we'll just ignore it
	}
	
	//Write a new freelist entry at the beginning of the freed block, and destroy the old "used" allocation entry at the end of the piece
	
	if(!closest_left)
	{
		new_entry->next = freelist_head;
		freelist_head = new_entry;
		new_entry_prev = NULL;
	}
	else
	{
		new_entry->next = closest_left->next;
		closest_left->next = new_entry;
		new_entry_prev = closest_left;
	}
	
	new_entry->size = p_entry->size;
	p_entry->size = 0;
	
	printf("New FL at %p, size: %zu, next: %p\n", (uchar*)new_entry, new_entry->size, new_entry->next);
	
	
	/************************************************/
	/*	Step 2: Merge with adjacent left piece	 	*/
	/************************************************/
	
	if(	closest_left && 
		(uchar*)closest_left - closest_left->size - sizeof(Heap_FreeList) == (uchar*)new_entry)
	{
		printf("Found left adjacent piece at %p. Size: %zu, Next: %p\n", (uchar*)closest_left, closest_left->size, closest_left->next);
		
		closest_left->size +=  new_entry->size + sizeof(Heap_FreeList);
		closest_left->next = new_entry->next;	
		new_entry->size = 0;
		new_entry->next = 0;
		new_entry = closest_left;				//in case if a right adjacent piece exists too
		new_entry_prev = closest_left_prev;
		
		printf("Merged! New Size: %zu, Next: %p\n", new_entry->size, new_entry->next);
	}
	
	/*Lower break if the new piece is at the very end*/
	
	
	/************************************************/
	/*	Step 3: Merge with adjacent right piece	 	*/
	/************************************************/
	
	//recall p_entry is the old address of the "used" allocation entry, which is now the end of the new free piece new_entry
	
	if(	closest_right && 
		(uchar*)closest_right + sizeof(Heap_FreeList) == (uchar*)p_entry)
	{
		printf("Found right adjacent piece at %p. Size: %zu, Next: %p\n", (uchar*)closest_right, closest_right->size, closest_right->next);
		
		new_entry->size += closest_right->size + sizeof(Heap_FreeList);
		new_entry->next = closest_right->next;	
		closest_right->size = 0;
		closest_right->next = 0;
		
		printf("Merged! New Size: %zu, Next: %p\n", new_entry->size, new_entry->next);
	}
	
	
	/************************************************/
	/*			Step 4: Reduce Malloc Break		 	*/
	/************************************************/
	
	//Only reduce the break if the last free piece is greater than one margin
	//Heap will only reduce/increase by exact margin sizes
	
	if(	(uchar*)new_entry - new_entry->size == malloc_break &&
		!new_entry->next)
	{
		//If the last free piece exceeds one margin
		if(new_entry->size > malloc_margin_size)
		{
			break_reduced = new_entry->size / malloc_margin_size;
			break_reduced *= malloc_margin_size;
			
			//Reduce the break, thus reducing the allocated heap
			new_entry->size -= break_reduced;
			malloc_break += break_reduced;
			printf("Reduced break by %zu bytes! New Size: %zu, Next: %p\n", break_reduced, new_entry->size, new_entry->next);
		}
		
		//If the last free piece has the exact size as one margin
		else if(new_entry->size + sizeof(Heap_FreeList) == malloc_margin_size)
		{
			new_entry->size = 0;
			new_entry_prev->next = NULL;
			
			malloc_break += malloc_margin_size;
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


