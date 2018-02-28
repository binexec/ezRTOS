/*
Note: malloc_heap_start > malloc_heap_end, since heap grows upwards
Adding an offset to a memory address will make it go down (shrinking), and subtracting will make it go up (growing).

This implementation of malloc saves heap space by minimizing the segment header (only stores segment size and next pointer), at the cost of runtime (no previous pointer).
*/

#include "my_malloc.h"

typedef unsigned char uchar;

uchar* malloc_heap_start;						//Start of the dynamic memory heap
uchar* malloc_heap_end;							//Absolute end of the memory segment for the heap; cannot allocate further than this

uchar* malloc_break;							//Also referred as "brk", the current end for the allocated heap	
size_t malloc_margin_size = 512;				//How many bytes to extend the heap at a time. One margin MUST be greater than sizeof(Heap_Seg)!
Heap_Seg *freelist_head;						//Head of the first heap free list entry


#define MAX_HEAP_SIZE	(size_t)(malloc_heap_start - malloc_heap_end)


/************************************************************************/
/*								HELPERS			 						*/
/************************************************************************/

/*Implement this function properly to retrieve the current process' stack pointer, if you want to prevent the heap from smashing into the stack*/
void* get_current_sp()
{
	return malloc_heap_end;
}

void check_alignment()
{
	size_t align_diff = (malloc_heap_start - malloc_break) % malloc_margin_size;
	printf("The break is currently %s. %zu\n", (align_diff == 0)? "aligned":"NOT aligned", align_diff);
}


/*Used by free() and realloc(), makes sure p is a valid pointer on the heap first*/
int pointer_is_valid(void* p)
{
	Heap_Seg *p_entry = p - sizeof(Heap_Seg);
	
	if(p == NULL)
		return 0;
	
	if((uchar*)p < malloc_break || (uchar*)p < malloc_heap_end || (uchar*)p > malloc_heap_start)
	{
		printf("p is not within the current heap range!\n");
		return 0;
	}
	
	if(p_entry->size >= MAX_HEAP_SIZE || p_entry->next != NULL)
	{
		printf("p does not seem to be a valid allocation entry!\n");
		printf("P: %p, Size: %zu, Next: %p\n", p, p_entry->size, p_entry->next);
		return 0;
	}
	
	return 1;
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
	
	//Is it necessary for our platform to have some canaries at the end of the heap? If so, add the initialization here
	
	printf("******Heap Start: %p, Heap End: %p\n\n", malloc_heap_start, malloc_heap_end);
	
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
	
	size_t shrunk_size;
	
	
	//Used for expanding malloc break;
	size_t bytes_needed;
	size_t align_diff;
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
		
		//Shrink the size of the original segment to accomodate the requested lengths and a new seg header
		next_smallest_piece->size -= len + sizeof(Heap_Seg);
		
		//Calculate the expected return address for the granted memory
		retaddr = (uchar*)next_smallest_piece + sizeof(Heap_Seg);			//The actual start of the original segment
		retaddr += next_smallest_piece->size + sizeof(Heap_Seg);			//The actual start of the splitted piece to be returned
		
		//Write a new allocation entry for the new splitted segment to be returned. 
		//The shrunken free piece is still at its original location (RIGHT side of the splitted/allocated piece)
		new_entry = (Heap_Seg*)(retaddr - sizeof(Heap_Seg));
		new_entry->size = len;
		new_entry->next = NULL;
																
		printf("Split an allocated piece of %zu bytes at %p\n", new_entry->size, retaddr);
		printf("New free piece at %p. Size: %zu, Next: %p\n", next_smallest_piece, next_smallest_piece->size, next_smallest_piece->next);
		return retaddr;
	}
	
	
	/************************************************/
	/*		Attempt 3: Allocate more heap space 	*/
	/************************************************/
	
	
	/*Reason 1: If heap is uninitialized or the allocated heap is full*/
	if(!freelist_head)
	{	
		//Calculate how many new bytes (including a new header) are needed
		bytes_needed = len + sizeof(Heap_Seg);
		
		//Calculate the return address for the request (in advance, assuming additional heap will be granted)
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
	
	/*Reason 2: The tailing piece of the allocated heap is free but not large enough*/
	else if(freelist_tail)
	{	
		printf("Tail_FL address: %p\n", freelist_tail);
		printf("Remaining allocated free heap: %zu\n", freelist_tail->size);

		//Calculate how many new bytes (including a new header) are needed
		bytes_needed = len - freelist_tail->size;
			
		//Calculate the return address for the request (in advance, assuming additional heap will be granted)
		retaddr = (uchar*)freelist_tail + sizeof(Heap_Seg);			//The actual start of freelist_tail's segment without header
		retaddr -= bytes_needed;									//The actual start of the expanded memory piece	

		//Because the tail piece will be enlarged, we'll wipe it as the header itself will be part of the granted memory
		freelist_tail->size = 0;
		freelist_tail->next = NULL;
	}
	
	
	//Align the new segment with the margin size
	align_diff = bytes_needed % malloc_margin_size;
	if(align_diff > 0)
	{
		//If there are additional free space after alignment, add in a new segment header. 
		//This may require the heap to be increased by another margin size

		bytes_needed += sizeof(Heap_Seg*);
		align_diff = bytes_needed % malloc_margin_size;
		bytes_needed += (align_diff > 0)? (malloc_margin_size - align_diff):0;
	}

	
	//Allocate additional heap space by pushing the break back
	new_break = malloc_break - bytes_needed;	
	if(new_break < malloc_heap_end || new_break < (uchar*)get_current_sp())
	{
		printf("Cannot continue. Allocation will exceed heap or smash into stack.\n");
		return NULL;
	}	
	malloc_break = new_break;
	printf("Malloc Break increased by: %zu. New break at %p\n", bytes_needed, malloc_break);
	
	
	//Write an allocation entry for the segment to be returned
	new_entry = (Heap_Seg*)(retaddr - sizeof(Heap_Seg));
	new_entry->size = len;			
	new_entry->next = NULL;
	printf("Allocated %zu bytes at %p\n", new_entry->size, retaddr);
	
	
	//Write a free segment entry at the malloc break, if there are free spaces left
	if(align_diff > 0)
	{
		new_entry = (Heap_Seg*)malloc_break;
		new_entry->size = ((uchar*)retaddr - sizeof(Heap_Seg)) - (malloc_break + sizeof(Heap_Seg));	
		new_entry->next = NULL;
		printf("New FL at: %p, size: %zu, Brkval: %p\n", new_entry, new_entry->size, malloc_break);
		
		//Update freelist chain
		if(freelist_tail_prev)
			freelist_tail_prev->next = new_entry;
		else
			freelist_head = new_entry;
	}
	else
	{
		//Update freelist chain
		if(freelist_tail_prev)
			freelist_tail_prev->next = NULL;
		else
			freelist_head = NULL;
	}
	
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
	
	size_t align_diff;
	size_t reduce_amount;
	uchar *tail_end;
	uchar *old_break;
	
	
	if(!pointer_is_valid(p))
		return;
	
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
	
	//printf("Closest Left at %p \nClosest Right at %p \n", closest_left, closest_right);
	
	printf("New FL at %p, size: %zu, next: %p\n", (uchar*)p_entry, p_entry->size, p_entry->next);
	
	
	/************************************************/
	/*	Step 2: Merge with adjacent left piece	 	*/
	/************************************************/
	
	if(	closest_left && 
		(uchar*)p_entry + sizeof(Heap_Seg) + p_entry->size == (uchar*)closest_left)
	{
		printf("Found left adjacent piece at %p. Size: %zu, Next: %p\n", (uchar*)closest_left, closest_left->size, closest_left->next);
		
		p_entry->size += closest_left->size + sizeof(Heap_Seg);
		closest_left->size = 0;
		closest_left->next = 0;
		
		if(closest_left_prev)
		{
			closest_left_prev->next = p_entry;
			p_entry_prev = closest_left_prev;
		}
		else
			freelist_head = p_entry;
		
		printf("Merged! New Size: %zu, Next: %p\n", p_entry->size, p_entry->next);
	}
	
	
	/************************************************/
	/*	Step 3: Merge with adjacent right piece	 	*/
	/************************************************/
	
	if(	closest_right && 
		(uchar*)closest_right + sizeof(Heap_Seg) + closest_right->size == (uchar*)p_entry)
	{
		printf("Found right adjacent piece at %p. Size: %zu, Next: %p\n", (uchar*)closest_right, closest_right->size, closest_right->next);
		
		closest_right->size += p_entry->size + sizeof(Heap_Seg);
		p_entry->size = 0;
		p_entry->next = 0;
		p_entry = closest_right;
		
		if(p_entry_prev)
			p_entry_prev->next = closest_right;
		else
			freelist_head = p_entry;
		
		printf("Merged! New Size: %zu, Next: %p\n", p_entry->size, p_entry->next);
	}
	
	
	/************************************************/
	/*			Step 4: Reduce Malloc Break		 	*/
	/************************************************/
	
	//Only reduce the break if the last free piece is greater than one margin
	//Heap will only reduce/increase by exact margin sizes
	
	if((uchar*)p_entry == malloc_break && !p_entry->next)
	{
		if(p_entry->size + sizeof(Heap_Seg) < malloc_margin_size)
		{
			printf("No need to reduce break at this time\n");
			return;
		}
		
		//Save the addresses of where the current malloc break is, and where the free piece at the malloc break ends
		old_break = (uchar*)p_entry;
		tail_end = (uchar*)p_entry + sizeof(Heap_Seg) + p_entry->size;
		
		//If the tail piece and its header is an exact multiple of the margin size
		align_diff = (p_entry->size + sizeof(Heap_Seg)) % malloc_margin_size;
		if(!align_diff)
		{			
			//Reduce the break right to the end, and no need to create a new free piece
			malloc_break = tail_end;
			
			if(p_entry_prev)
				p_entry_prev->next = NULL;
			else
				freelist_head = NULL;

			printf("Break Reduction will eliminate the tail piece!\n");
			printf("New Break at %p\n. Reduced by %zu bytes\n", malloc_break, malloc_break - old_break);
		}
		else
		{
			//Reduce the break while maintaining alignment
			reduce_amount = (p_entry->size + sizeof(Heap_Seg)) - align_diff; 
			malloc_break += reduce_amount;		
			printf("Break to be reduced by %zu bytes\n", reduce_amount);

			//Write a new free entry at malloc break
			p_entry = (Heap_Seg*)malloc_break;
			p_entry->size = tail_end - (malloc_break + sizeof(Heap_Seg));
			p_entry->next = NULL;
			
			//Update freelist
			if(p_entry_prev)
				p_entry_prev->next = p_entry;
			else
				freelist_head = p_entry;
		}
		
		//Erase the free seg entry at the old malloc break
		p_entry = (Heap_Seg*)old_break;
		p_entry->size = 0;
		p_entry->next = NULL;
		
		printf("New Break at %p, Size: %zu, Next: %p\n", malloc_break, p_entry->size, p_entry->next);
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
	uchar* retaddr;
	
	
	if(!pointer_is_valid(p))
		return NULL;
	
	printf("(REALLOC %p) P_size: %zu, P_next: %p\n", p, p_entry->size, p_entry->next);
	
	
	/************************************************/
	/*			Step 1a: If we're shrinking		 	*/
	/************************************************/
	
	size_diff = len - p_entry->size;
	
	printf("Size difference: %d\n", size_diff);
	
	if(size_diff == 0)
		return p;
	
	else if(size_diff < 0)
	{
		//Make size_diff positive
		size_diff = p_entry->size - len;
		
		//Don't shrink if the size difference isn't big enough to insert a new segment header
		if(size_diff <= sizeof(Heap_Seg))
		{
			printf("Size difference is too insignificant, skip shrinking...\n");
			return p;
		}
		
		retaddr = (uchar*)p + size_diff;
		
		//Write a new segment entry above the requested length
		p_entry = (Heap_Seg*)(retaddr - sizeof(Heap_Seg));
		p_entry->size = len;
		p_entry->next = NULL;
		
		//Rewrite the old segment header and mark it as free
		old_entry->size = size_diff - sizeof(Heap_Seg);
		my_free(p);	
		
		return retaddr;
	}
	
	
	/************************************************/
	/*			Step 1b: If we're growing		 	*/
	/************************************************/
	
	
	
	return NULL;
}



























