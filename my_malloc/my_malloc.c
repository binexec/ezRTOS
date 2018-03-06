/*
Note: malloc_heap_start > malloc_heap_end, since heap grows upwards
Adding an offset to a memory address will make it go down (shrinking), and subtracting will make it go up (growing).

This implementation of malloc saves heap space by minimizing the segment header (only stores segment size and next pointer), at the cost of runtime (no previous pointer).
*/

#include <string.h>
#include "my_malloc.h"

typedef unsigned char uchar;

uchar* malloc_heap_start;						//Start of the dynamic memory heap
uchar* malloc_heap_end;							//Absolute end of the memory segment for the heap; cannot allocate further than this

uchar* malloc_break;							//Also referred as "brk", the current end for the allocated heap	
Heap_Seg *freelist_head;						//Head of the first heap free list entry



/************************************************************************/
/*								HELPERS			 						*/
/************************************************************************/

#define MAX_HEAP_SIZE	(size_t)(malloc_heap_start - malloc_heap_end)

#define segment_end(p)	((uchar*)p + sizeof(Heap_Seg) + p->size)

/*Implement this function properly to retrieve the current process' stack pointer, if you want to prevent the heap from smashing into the stack*/
static void* get_current_sp()
{
	return malloc_heap_end;
}


/*Used by free() and realloc(), makes sure p is a valid pointer on the heap first*/
static int pointer_is_valid(void* p)
{
	Heap_Seg *p_entry = p - sizeof(Heap_Seg);
	
	if(p == NULL)
		return 0;
	
	//Make sure p is within the heap's bound
	if((uchar*)p < malloc_break || (uchar*)p < malloc_heap_end || (uchar*)p > malloc_heap_start)
	{
		printf("p is not within the current heap range!\n");
		return 0;
	}
	
	//Make sure p's allocation entry fields appears "sane"
	if(p_entry->size >= MAX_HEAP_SIZE || p_entry->next != NULL)
	{
		printf("p does not seem to be a valid allocation entry!\n");
		printf("P: %p, Size: %zu, Next: %p\n", p, p_entry->size, p_entry->next);
		return 0;
	}
	
	return 1;
}


static inline void write_seg_header(void* seg_header, size_t len, Heap_Seg* next)
{
	Heap_Seg* new_entry =  (Heap_Seg*)seg_header;
	new_entry->size = len;
	new_entry->next = next;
}


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
	
	printf("Heap Start: %p, Heap End: %p\n\n", malloc_heap_start, malloc_heap_end);
	return 1;
}


Malloc_Param save_malloc_param(void)
{
	Malloc_Param p;
	
	p.malloc_heap_start 	= malloc_heap_start;
	p.malloc_heap_end 		= malloc_heap_end;
	p.malloc_break 			= malloc_break;
	p.freelist_head 		= freelist_head;
	
	return p;
}


void load_malloc_param(Malloc_Param p)
{
	malloc_heap_start 	= p.malloc_heap_start;
	malloc_heap_end 	= p.malloc_heap_end;
	malloc_break 		= p.malloc_break;
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
	
	uchar* retaddr = NULL;
	Heap_Seg *new_entry;
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
		
		//Shrink the size of the original segment to accomodate the requested lengths and a new seg header
		next_smallest_piece->size -= len + sizeof(Heap_Seg);
		
		//Calculate the expected return address for the granted memory
		retaddr = (uchar*)next_smallest_piece + sizeof(Heap_Seg);			//The actual start of the original segment
		retaddr += next_smallest_piece->size + sizeof(Heap_Seg);			//The actual start of the splitted piece to be returned
		
		//Write a new allocation entry for the new splitted segment to be returned. 
		//The shrunken free piece is still at its original location (RIGHT side of the splitted/allocated piece)
		write_seg_header(retaddr - sizeof(Heap_Seg), len, NULL);
		printf("New allocated piece at %p, %zu bytes\n", retaddr, len);
		printf("Split a new free piece at %p. Size: %zu, Next: %p\n", next_smallest_piece, next_smallest_piece->size, next_smallest_piece->next);
		
		return retaddr;
	}
	
	
	/************************************************/
	/*		Attempt 3: Allocate more heap space 	*/
	/************************************************/
	
	//Calculate the return address for the request (in advance, assuming additional heap will be granted)
	
	if(malloc_break == malloc_heap_start)				//Heap is currently unallocated
		retaddr = malloc_heap_start - len;				
	else												//No existing free pieces available on the heap
		retaddr = malloc_break - len;

	
	//Allocate additional heap space needed for the requested length and a new header
	new_break = malloc_break - (len + sizeof(Heap_Seg));	
	if(new_break < malloc_heap_end || new_break < (uchar*)get_current_sp())
	{
		printf("Cannot continue. Allocation will exceed heap or smash into stack.\n");
		return NULL;
	}	
	malloc_break = new_break;
	printf("Malloc Break increased by: %zu bytes. New break at %p\n", len + sizeof(Heap_Seg), malloc_break);
	
	//Write an allocation entry for the segment to be returned
	write_seg_header(retaddr - sizeof(Heap_Seg), len, NULL);
	printf("Allocated piece at %p, %zu bytes\n", retaddr, len);
	
	return retaddr;
}










/************************************************************************/
/*								CALLOC		  							*/
/************************************************************************/

void* my_calloc(size_t nitems, size_t size)
{
	size_t total_len = nitems * size;
	void *retaddr = my_malloc(total_len); 
	
	if(!retaddr)
		return NULL;
	memset(retaddr, 0, total_len);
	
	return retaddr;
}










/************************************************************************/
/*								FREE		  							*/
/************************************************************************/

void my_free(void *p)
{
	Heap_Seg *p_entry = p - sizeof(Heap_Seg);
	Heap_Seg *p_entry_prev = NULL;
	
	Heap_Seg *current_piece = NULL;
	Heap_Seg *closest_left = NULL, *closest_left_prev = NULL;
	Heap_Seg *closest_right = NULL;
	
	
	if(!pointer_is_valid(p))
		return;
	
	printf("(FREEING %p) P_size: %zu, P_next: %p\n", p, p_entry->size, p_entry->next);
	
	/************************************************/
	/*		Step 1: Freeing the requested piece 	*/
	/************************************************/
	
	//Iterate through the current freelist and locate the closest neighbouring free segments to p
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
		else
		{
			//There should never be an occurance where p_entry is in the freelist. This might be a double free attempt
			printf("Double free detected!\n");
			return;
		}
		
	}
	
	//Write a new freelist entry at the beginning of the freed block
	if(!closest_left)
	{
		p_entry->next = freelist_head;
		freelist_head = p_entry;
	}
	else
	{
		p_entry->next = closest_left->next;
		closest_left->next = p_entry;
		p_entry_prev = closest_left;
	}
	
	printf("New FL at %p, size: %zu, next: %p\n", (uchar*)p_entry, p_entry->size, p_entry->next);
	
	
	/************************************************/
	/*	Step 2: Merge with adjacent left piece	 	*/
	/************************************************/
	
	if(closest_left && segment_end(p_entry) == (uchar*)closest_left)
	{
		printf("Found left adjacent piece at %p. Size: %zu, Next: %p\n", (uchar*)closest_left, closest_left->size, closest_left->next);
		
		p_entry->size += closest_left->size + sizeof(Heap_Seg);
		printf("Merged! New Size: %zu, Next: %p\n", p_entry->size, p_entry->next);
		
		//Erase old header
		write_seg_header(closest_left, 0, NULL);
		
		//Update freelist
		if(closest_left_prev)
		{
			closest_left_prev->next = p_entry;
			p_entry_prev = closest_left_prev;
		}
		else
			freelist_head = p_entry;
	}
	
	
	/************************************************/
	/*	Step 3: Merge with adjacent right piece	 	*/
	/************************************************/
	
	if(closest_right && segment_end(closest_right) == (uchar*)p_entry)
	{
		printf("Found right adjacent piece at %p. Size: %zu, Next: %p\n", (uchar*)closest_right, closest_right->size, closest_right->next);
		
		closest_right->size += p_entry->size + sizeof(Heap_Seg);
		write_seg_header(p_entry, 0, NULL);
		p_entry = closest_right;
		printf("Merged! New Size: %zu, Next: %p\n", p_entry->size, p_entry->next);
		
		//Update freelist
		if(p_entry_prev)
			p_entry_prev->next = closest_right;
		else
			freelist_head = p_entry;
	}
	
	
	/************************************************/
	/*			Step 4: Reduce Malloc Break		 	*/
	/************************************************/
	
	if((uchar*)p_entry == malloc_break && !p_entry->next)
	{
		//Reduce the break to where the tail piece ends
		malloc_break = segment_end(p_entry);
		printf("New Break at %p\n", malloc_break);
		
		write_seg_header(p_entry, 0, NULL);
		
		//Update freelist
		if(p_entry_prev)
			p_entry_prev->next = NULL;
		else
			freelist_head = NULL;
	}
}










/************************************************************************/
/*								REALLOC		  							*/
/************************************************************************/

void* my_realloc(void *p, size_t len)
{
	Heap_Seg *p_entry = p - sizeof(Heap_Seg);
	Heap_Seg *p_entry_prev = NULL;
	Heap_Seg *old_entry = p_entry;
	
	int size_diff;
	uchar* retaddr = NULL;
	
	Heap_Seg *current_piece = NULL;
	Heap_Seg *closest_right = NULL;
	
	
	if(!pointer_is_valid(p))
		return NULL;
	printf("(REALLOC %p) P_size: %zu, P_next: %p\n", p, p_entry->size, p_entry->next);
	
	
	/****************************************/
	/*				Shrinking				*/
	/****************************************/
	
	size_diff = len - p_entry->size;
	
	if(size_diff == 0)
		return p;
	
	else if(size_diff < 0)
	{
		size_diff = p_entry->size - len;		//Make size_diff positive
		
		//Don't shrink if the size difference isn't big enough to insert a new segment header
		if(size_diff <= sizeof(Heap_Seg))
		{
			printf("Size difference %d is too insignificant, skip shrinking...\n", size_diff);
			return p;
		}
		
		retaddr = (uchar*)p + size_diff;
		
		//Write a new segment entry above the requested length
		write_seg_header(retaddr - sizeof(Heap_Seg), len, NULL);
		printf("Shrunk segment at %p, %zu bytes\n", retaddr, len);
		
		//Rewrite the old segment header and mark it as free
		old_entry->size = size_diff - sizeof(Heap_Seg);
		printf("Creating a free entry at %p, size: %zu\n", old_entry, old_entry->size);
		my_free(p);	
		
		return retaddr;
	}
	
	
	/****************************************/
	/*			Growing In-place			*/
	/****************************************/

	//Iterate the freelist and find the closest free piece on the right to p
	for(current_piece = freelist_head; current_piece; current_piece = current_piece->next)
	{
		if(current_piece < p_entry)
		{
			closest_right = current_piece;
			break;
		}
	}

	
	/*Scenario: The expanding piece has a free chunk directly above itself, and is big enough to merge with*/
	if(closest_right && segment_end(closest_right) == (uchar*)p_entry)
	{	
		//Merging with top piece if it fits exactly
		if(closest_right->size + sizeof(Heap_Seg) == size_diff)
		{
			retaddr = (uchar*)closest_right + sizeof(Heap_Seg);
			printf("Exact Merge!\n");
		}
		
		//Merging with top piece yields excess free spaces
		else if(closest_right->size >= size_diff)
		{
			retaddr = (uchar*)p - size_diff;
			
			//Also update the free piece's original entry, if it has more free space still
			if(closest_right->size > size_diff)
				closest_right->size -= size_diff;
			
			printf("Updated free entry at %p, size: %zu, next: %p\n", closest_right-sizeof(Heap_Seg), closest_right->size, closest_right->next);
		}
	}
	
	/*Scenario: The expanding piece is at the malloc break*/
	else if((uchar*)p_entry == malloc_break)
	{
		//If the requested is at the malloc break, simply expand it to fulfil the requested length
		retaddr = (uchar*)p - size_diff;
		
		malloc_break -= size_diff;		
		p_entry = (Heap_Seg*)malloc_break;
		printf("New break at %p\n", malloc_break);
	}

	if(retaddr)
	{
		//Write a new segment entry above the requested length
		write_seg_header(p_entry, len, NULL);
		printf("Expanded segment at %p, %zu bytes\n", retaddr, len);	
			
		//Wipe the old seg entry, as it's now part of the allocated memory
		old_entry->size = 0;
		old_entry->next = NULL;
			
		return retaddr;	
	}

	
	/****************************************/
	/*		New Allocation for growth		*/
	/****************************************/
	
	printf("A new segment will be created for reallocation..\n");
	
	retaddr = my_malloc(len);
	if(!retaddr) 
		return NULL;
	memcpy(p, retaddr, p_entry->size);
	my_free(p);

	return retaddr;
}


#undef MAX_HEAP_SIZE
#undef segment_end
























