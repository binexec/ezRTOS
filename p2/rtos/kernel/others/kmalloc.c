/*
Note: malloc_heap_start > malloc_heap_end, since heap grows upwards
Adding an offset to a memory address will make it go down (shrinking), and subtracting will make it go up (growing).

This implementation of malloc saves heap space by minimizing the segment header (only stores segment size and next pointer), at the cost of runtime (no previous pointer).

For an annotated version of this file, please visit https://github.com/binexec/DynMemAllocator
*/

#include "kmalloc.h"
#include <string.h>

typedef unsigned char uchar;

static uchar* kmalloc_heap_start;						//Start of the dynamic memory heap
static uchar* kmalloc_heap_end;							//Absolute end of the memory segment for the heap; cannot allocate further than this

static uchar* kmalloc_break;							//Also referred as "brk", the current end for the allocated heap	
static Heap_Seg *freelist_head;							//Head of the first heap free list entry



/************************************************************************/
/*								HELPERS			 						*/
/************************************************************************/

#define MAX_HEAP_SIZE	(size_t)(kmalloc_heap_start - kmalloc_heap_end)

#define segment_end(p)	((uchar*)p + sizeof(Heap_Seg) + p->size)


static int check_stack_integrity(void* new_break)
{
	return 1;
}


static int pointer_is_valid(void* p)
{
	Heap_Seg *p_entry = p - sizeof(Heap_Seg);
	
	if(p == NULL)
		return 0;
	
	if((uchar*)p < kmalloc_break || (uchar*)p < kmalloc_heap_end || (uchar*)p > kmalloc_heap_start)
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


static inline void write_seg_header(void* seg_header, size_t len, Heap_Seg* next)
{
	Heap_Seg* new_entry =  (Heap_Seg*)seg_header;
	new_entry->size = len;
	new_entry->next = next;
}


//Similar to sbrk() in unix
static void* grow_kmalloc_break(size_t amount)
{
	uchar* new_break = kmalloc_break - amount;
	
	if(new_break < kmalloc_heap_end || new_break > kmalloc_heap_start || !check_stack_integrity(new_break))
	{
		printf("Cannot continue. Allocation will exceed heap or smash into stack.\n");
		return NULL;
	}	
	
	kmalloc_break = new_break;
	return kmalloc_break;
}


//Similar to brk() in unix
void* get_kmalloc_break()
{
	return kmalloc_break;
}


/************************************************************************/
/*							INITIALIZATION		 						*/
/************************************************************************/

int init_kmalloc(uchar* start, uchar* end)
{
	if(start < end)
	{
		printf("Heap starting address must be greater than end address!\n");
		return 0;
	}
	
	kmalloc_heap_start 	= start;
	kmalloc_heap_end 	= end;
	kmalloc_break 		= kmalloc_heap_start;	
	freelist_head 		= NULL;
	
	printf("Heap Start: %p, Heap End: %p\n\n", kmalloc_heap_start, kmalloc_heap_end);
	return 1;
}


kMalloc_Param save_kmalloc_param(void)
{
	kMalloc_Param p;
	
	p.kmalloc_heap_start 	= kmalloc_heap_start;
	p.kmalloc_heap_end 		= kmalloc_heap_end;
	p.kmalloc_break 			= kmalloc_break;
	p.freelist_head 		= freelist_head;
	
	return p;
}


void load_kmalloc_param(kMalloc_Param p)
{
	kmalloc_heap_start 	= p.kmalloc_heap_start;
	kmalloc_heap_end 	= p.kmalloc_heap_end;
	kmalloc_break 		= p.kmalloc_break;
	freelist_head 		= p.freelist_head;
}











/************************************************************************/
/*								MALLOC		  							*/
/************************************************************************/

void* kmalloc(size_t len)
{
	
	Heap_Seg *current_piece = NULL, *previous_piece = NULL;
	Heap_Seg *exact_piece = NULL, *exact_piece_prev = NULL;
	Heap_Seg *next_smallest_piece = NULL;
	
	uchar* retaddr = NULL;


	/*Attempt 1: Find an exact piece*/
	
	for(current_piece = freelist_head; current_piece; current_piece = current_piece->next, previous_piece = current_piece)
	{
		if(current_piece->size == len)
		{
			exact_piece = current_piece;
			exact_piece_prev = previous_piece;
			break;
		}
		
		if(current_piece->size > len + sizeof(Heap_Seg) && (!next_smallest_piece || current_piece->size < next_smallest_piece->size))
			next_smallest_piece = current_piece;
	}

	if(exact_piece)
	{
		retaddr = (uchar*)exact_piece + sizeof(Heap_Seg);
		
		if(exact_piece_prev)
			exact_piece_prev->next = exact_piece->next;
		else
			freelist_head = exact_piece->next;	
		exact_piece->next = NULL;
		
		return retaddr;
	}
	
	
	/*Attempt 2: Split an larger piece*/

	if(next_smallest_piece)
	{
		next_smallest_piece->size -= len + sizeof(Heap_Seg);
		
		retaddr = (uchar*)next_smallest_piece + sizeof(Heap_Seg);
		retaddr += next_smallest_piece->size + sizeof(Heap_Seg);

		write_seg_header(retaddr - sizeof(Heap_Seg), len, NULL);
		
		return retaddr;
	}
	
	
	/*Attempt 3: Allocate more heap space*/
	
	if(kmalloc_break == kmalloc_heap_start)				//Heap is currently unallocated
		retaddr = kmalloc_heap_start - len;				
	else												//No existing free pieces available on the heap
		retaddr = kmalloc_break - len;

	if(!grow_kmalloc_break(len + sizeof(Heap_Seg)))
		return NULL;
		
	write_seg_header(retaddr - sizeof(Heap_Seg), len, NULL);	
	
	return retaddr;
}










/************************************************************************/
/*								CALLOC		  							*/
/************************************************************************/

void* kcalloc(size_t nitems, size_t size)
{
	size_t total_len = nitems * size;
	void *retaddr = kmalloc(total_len); 
	
	if(!retaddr)
		return NULL;
	memset(retaddr, 0, total_len);
	
	return retaddr;
}










/************************************************************************/
/*								FREE		  							*/
/************************************************************************/

void kfree(void *p)
{
	Heap_Seg *p_entry = p - sizeof(Heap_Seg);
	Heap_Seg *p_entry_prev = NULL;
	
	Heap_Seg *current_piece = NULL;
	Heap_Seg *closest_left = NULL, *closest_left_prev = NULL;
	Heap_Seg *closest_right = NULL;
	
	
	if(!pointer_is_valid(p))
		return;
	
	
	/*Step 1: Freeing the requested piece*/
	
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
			printf("Double free detected!\n");
			return;
		}
	}
	
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
	
	
	/*Step 2: Merge with adjacent left piece*/
	
	if(closest_left && segment_end(p_entry) == (uchar*)closest_left)
	{
		p_entry->size += closest_left->size + sizeof(Heap_Seg);
		write_seg_header(closest_left, 0, NULL);					//Erase old header
		
		if(closest_left_prev)
		{
			closest_left_prev->next = p_entry;
			p_entry_prev = closest_left_prev;
		}
		else
			freelist_head = p_entry;
	}
	
	
	/*Step 3: Merge with adjacent right piece*/
	
	if(closest_right && segment_end(closest_right) == (uchar*)p_entry)
	{
		closest_right->size += p_entry->size + sizeof(Heap_Seg);
		write_seg_header(p_entry, 0, NULL);
		p_entry = closest_right;
		
		if(p_entry_prev)
			p_entry_prev->next = closest_right;
		else
			freelist_head = p_entry;
	}
	
	
	/*Step 4: Reduce Malloc Break*/
	
	if((uchar*)p_entry == kmalloc_break && !p_entry->next)
	{
		kmalloc_break = segment_end(p_entry);
		write_seg_header(p_entry, 0, NULL);
		
		if(p_entry_prev)
			p_entry_prev->next = NULL;
		else
			freelist_head = NULL;
	}
}










/************************************************************************/
/*								REALLOC		  							*/
/************************************************************************/

void* krealloc(void *p, size_t len)
{
	Heap_Seg *p_entry = p - sizeof(Heap_Seg);
	Heap_Seg *old_entry = p_entry;
	
	int size_diff;
	uchar* retaddr = NULL;
	
	Heap_Seg *current_piece = NULL;
	Heap_Seg *closest_right = NULL;
	
	
	if(!pointer_is_valid(p))
		return NULL;
	
	/*Shrinking*/
	
	size_diff = len - p_entry->size;
	
	if(size_diff == 0)
		return p;
		
	else if(size_diff < 0)
	{
		size_diff = p_entry->size - len;		//Make size_diff positive
		
		if(size_diff <= sizeof(Heap_Seg))
			return p;

		retaddr = (uchar*)p + size_diff;
		write_seg_header(retaddr - sizeof(Heap_Seg), len, NULL);
		old_entry->size = size_diff - sizeof(Heap_Seg);
		kfree(p);	
		
		return retaddr;
	}
	
	
	/*Growing In-place*/

	for(current_piece = freelist_head; current_piece; current_piece = current_piece->next)
	{
		if(current_piece < p_entry)
		{
			closest_right = current_piece;
			break;
		}
	}


	if(closest_right && segment_end(closest_right) == (uchar*)p_entry)
	{	
		if(closest_right->size + sizeof(Heap_Seg) == size_diff)
			retaddr = (uchar*)closest_right + sizeof(Heap_Seg);
			
		else if(closest_right->size >= size_diff)
		{
			retaddr = (uchar*)p - size_diff;

			if(closest_right->size > size_diff)
				closest_right->size -= size_diff;
		}
	}
	else if((uchar*)p_entry == kmalloc_break)
	{
		retaddr = (uchar*)p - size_diff;
		
		if(!grow_kmalloc_break(len + sizeof(Heap_Seg)))
			return NULL;
		p_entry = (Heap_Seg*)kmalloc_break;
	}
	

	if(retaddr)
	{
		write_seg_header(p_entry, len, NULL);
		old_entry->size = 0;
		old_entry->next = NULL;
			
		return retaddr;	
	}

	
	/*New Allocation for growth*/
	
	retaddr = kmalloc(len);
	if(!retaddr) 
		return NULL;
	memcpy(p, retaddr, p_entry->size);
	kfree(p);

	return retaddr;
}


#undef MAX_HEAP_SIZE
#undef segment_end
























