/*
 * kernel_extra.c - Project 2 Extra Credit, CMPSC 473
 * Copyright 2023 Ruslan Nikolaev <rnikola@psu.edu>
 * Distribution, modification, or usage without explicit author's permission
 * is not allowed.
 */

#include <malloc.h>
#include <types.h>
#include <string.h>
#include <printf.h>

// Your mm_init(), malloc(), free() code from mm.c here
// You can only use mem_sbrk(), mem_heap_lo(), mem_heap_hi() and
// Project 2's kernel headers provided in 'include' such
// as memset and memcpy.
// No other files from Project 1 are allowed!

/* What is the correct alignment? */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x + ALIGNMENT - 1) / ALIGNMENT);
}

/* Global variables */
static char *heap_bptr;  			/* Pointer to first block */
struct FLNode free_list;			/* Struct for doubly linked list */
typedef struct FLNode * FL_Pointer;		/* Struct for node pointer in the DLL */


struct FLNode {
  struct FLNode *next_node;	/* Pointer for previous node */
  struct FLNode *prev_node;	/* Pointer for next node */
};


/* Initialize the root node of a circular free list. The next & prev pointing to the root node itself. */
void FL_init (FL_Pointer root_node)
{
  root_node -> next_node = root_node;
  root_node -> prev_node = root_node;
}

/* Append after end node in the list. Usually, end_node will be the freelist struct. */
void FL_append (FL_Pointer end_node, FL_Pointer newitem)
{
  newitem -> next_node = end_node -> next_node;	/* Update next node of newitem using next node of end node (start node), making next node as new end */
  newitem -> prev_node = end_node;	/* Link previous node of newitem to end node */
  end_node -> next_node = newitem;	/* Link next node of end node to newitem */
  newitem -> next_node -> prev_node = newitem;	/* Link the start node to new end node */
}

/* Unlink the element at ptr address. ptr should NEVER be the root node or freelist head since access of other node will be lost.  */
void FL_unlink(struct FLNode *ptr)
{
  /* Link previous node of ptr to next node of ptr, unlink ptr from previous node*/
  ptr -> prev_node -> next_node = ptr -> next_node;	
  ptr -> next_node -> prev_node = ptr -> prev_node;
  /* Unlink ptr from next node */
  ptr -> next_node = NULL;
  ptr -> prev_node = NULL;
}

/* Internal Helper Routines */

/* Read a word at pointer address ptr */
static size_t READ_w(void* ptr)
{
    return *(size_t *)ptr;
}
/* Write a word at pointer address ptr*/
static void WRITE_w(void* ptr, size_t value)
{
    *(size_t *)ptr = value;
}
/* Read size from pointer address ptr */
static size_t READ_w_SIZE(void* ptr)
{
    return *(size_t *)ptr & ~0x7;
}
/* Read allocated fields from pointer address ptr */
static size_t READ_w_ALLOC(void* ptr)
{
    return *(size_t *)ptr & 0x1;
}
/* Compute address of header at pointer address ptr */
static void* Compute_HD(void* ptr)
{
    return ((char *)ptr) - (sizeof(void*));
} 
/* Compute address of footer at pointer address ptr */
static void* Compute_FT(void* ptr)
{
    return ((char *)ptr + READ_w_SIZE(Compute_HD(ptr)) - 2 * sizeof(void*));
} 
/* Compute address of next block at pointer address ptr */
static void* Compute_NXBP(void* ptr)
{
    return (char *)ptr + READ_w_SIZE((char *)ptr - (sizeof(void*)));
}
/* Compute address of previous block at pointer address ptr */
static void* Compute_PVBP(void* ptr)
{
    return (char *)ptr - READ_w_SIZE((char *)ptr - (2 * sizeof(void*)));
}
/* Pack a size and allocated bit status into a word */
static size_t Pack_w(size_t size, int alloc_bit)
{
    return ((size) | (alloc_bit));
}
/* Coalesce function, follow boundary tag coalescing guidelines */
static void *coalesce(void *ptr) 
   {	/* Coalesce function, follow boundary tag coalescing guidelines */
    size_t prev_block_allocation;
    size_t next_block_allocation;
    /* printf("%p ptr: \n", ptr);
    printf("%p Compute_PVBP: \n", Compute_PVBP(ptr));
    printf("%p Compute_NXBP: \n", Compute_NXBP(ptr));
    printf("%p mem_heap_lo: \n", mem_heap_lo());
    printf("%p mem_heap_hi: \n", mem_heap_hi());
    printf("%p Compute_FT(Compute_PVBP(ptr)): \n", Compute_FT(Compute_PVBP(ptr))); */
    
    if (Compute_PVBP(ptr) >= mem_heap_lo()) /*Check to see if prev block is in range of heap*/
    {
        prev_block_allocation = READ_w_ALLOC(Compute_FT(Compute_PVBP(ptr))); /* Read the allocation status of the previous pointer address */
    }
    else{
        prev_block_allocation = 1;
    }
    if (Compute_NXBP(ptr) <= mem_heap_hi()) /*Check to see if prev block is in range of heap*/
    {
        next_block_allocation = READ_w_ALLOC(Compute_HD(Compute_NXBP(ptr)));	/* Read the allocation status of the next pointer address */
    }
     else{
        next_block_allocation = 1;
    }
    size_t size = READ_w_SIZE(Compute_HD(ptr));	/* Read the size of the block at the pointer address */
    if (prev_block_allocation && next_block_allocation)
    {       /* Check if prev and next block are both allocated*/
        /* If is TRUE then add block to free list */
        FL_append(&free_list, ptr);
    }
    else if (prev_block_allocation && !next_block_allocation)
    {      /* Check if only next block is not allocated*/
    	/* If is TRUE then unlink next block first, add current block to free list */
        FL_unlink(Compute_NXBP(ptr));
        FL_append(&free_list, ptr);
        size += READ_w_SIZE(Compute_HD(Compute_NXBP(ptr)));	/* Add the size of the next block to the size of the current block */
        WRITE_w(Compute_HD(ptr), Pack_w(size, 0));	/* Update the size stored in header with new size at pointer address */
        WRITE_w(Compute_FT(ptr), Pack_w(size, 0));	/* Update the size stored in footer with new size at pointer address */
    }
    else if (!prev_block_allocation && next_block_allocation)
    {      /* Check if only previous block is not allocated*/
        size += READ_w_SIZE(Compute_HD(Compute_PVBP(ptr))); /* Add the size of the previous block to the size of the current block */
        WRITE_w(Compute_FT(ptr), Pack_w(size, 0)); /* Update the size stored in footer with new size at pointer address */
        WRITE_w(Compute_HD(Compute_PVBP(ptr)), Pack_w(size, 0)); /* Update the size stored in header with new size at previous pointer address */
        ptr = Compute_PVBP(ptr); /* Change the address of the new free blocks as the previous pointers address */
    }
    else
    {	/* Check if both blocks are not allocated*/
        /* If is TRUE then unlink next block */    
        FL_unlink(Compute_NXBP(ptr));
        size += (READ_w_SIZE(Compute_HD(Compute_PVBP(ptr))) +  READ_w_SIZE(Compute_FT(Compute_NXBP(ptr)))); /*Adding the size of the previous and next blocks to the size of the current block */
        WRITE_w(Compute_HD(Compute_PVBP(ptr)), Pack_w(size, 0)); /* Update the size stored in header with new size at previous pointer address */
        WRITE_w(Compute_FT(Compute_NXBP(ptr)), Pack_w(size, 0)); /* Update the size stored in footer with new size at next pointer address */ 
        ptr = Compute_PVBP(ptr); /* Return the previous pointers address as the new free blocks address */
    }
    return ptr;
}

/* Function for requesting memory and add it in the heap */
static void *mem_request(size_t size_req) 
{
    char *ptr;
    ptr = mem_sbrk(size_req);
    if (ptr == NULL) /* Validity check for content of block pointer */
    {
        return NULL; 
    }
    /* Create a free block with size of size_req to extend the heap block */
    WRITE_w(Compute_HD(ptr), Pack_w(size_req, 0));
    WRITE_w(Compute_FT(ptr), Pack_w(size_req, 0));
    WRITE_w(Compute_HD(Compute_NXBP(ptr)), Pack_w(0, 1));
    /* Coalesce if there is free blocks before it */
	/* Deallocate memory if not needed */
    return coalesce(ptr); 
}
// /* Function used in finding fit in list with first-fit method */
static void *first_fit(size_t size) 
{
    FL_Pointer ptr;
    /* Run though free list from next node after the root, then stop when run back to the root */
    for (ptr = free_list.next_node; ptr != &free_list; ptr = ptr->next_node){
        if ( (size <= READ_w_SIZE(Compute_HD(ptr)))) {
            return ptr;
        }
    }
     return NULL;
}
/* Function to set block as allocated depends on size of allocation */
static void allocate_block(void *ptr, size_t size)
{
    size_t size_r = READ_w_SIZE(Compute_HD(ptr));
    if ((size_r - size) >= 4 * sizeof(void*))
       {
           /* Block split*/
           /* Mark header and footer of block with size_a as allocated */
           /* Unlink free block from free list */
           FL_unlink(ptr);
           WRITE_w(Compute_HD(ptr), Pack_w(size, 1));
           WRITE_w(Compute_FT(ptr), Pack_w(size, 1));
           /* Mark header and footer of rest as free */
           ptr = Compute_NXBP(ptr);
           WRITE_w(Compute_HD(ptr), Pack_w((size_r - size), 0));
           WRITE_w(Compute_FT(ptr), Pack_w((size_r - size), 0));
           /* Link remaining free block to free list */
           FL_append(&free_list, ptr);
       }
    else
       {
       	   /*if leftover space is not enough */
       	   /* Unlink free block from free list */
       	   FL_unlink(ptr);
       	   /* Mark header and footer of whole free block as allocated */
	   WRITE_w(Compute_HD(ptr), Pack_w(size_r, 1));
	   WRITE_w(Compute_FT(ptr), Pack_w(size_r, 1));
       }
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    // printf("mm_init()");
    FL_init(&free_list);
    size_t CHUCKSIZE = (1 << 12);
    /* Create initial empty heap */
    heap_bptr = mem_sbrk(4 * sizeof(void*));
    if (heap_bptr == NULL) /* Vaildity check */
    {
    	return false;
    }
    WRITE_w(heap_bptr, 0); /* Alignment padding */
    WRITE_w(heap_bptr + ((sizeof(void*))), Pack_w((2 * sizeof(void*)), 1)); /* Write the start header */
    WRITE_w(heap_bptr + (2 * sizeof(void*)), Pack_w((2 * sizeof(void*)), 1)); /* Write the start footer*/
    WRITE_w(heap_bptr + (3 * sizeof(void*)), Pack_w(0, 1)); /* Write the end header */
    heap_bptr += 2 * sizeof(void*); 
    /* Set size of the heap */
    // printf("exiting mm_init");
    if (mem_request(CHUCKSIZE) == NULL) /* Vaildity check */
    {  
    	return false;
    }
    return true;    
    
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    // printf("malloc(%lu)", size);
    char *ptr;
    size_t size_a; /* Size for alignment */
    if (size <= 0)	/* Skip and return NULL if size is invalid */
    {
        return NULL;
    }
    size_a = (size <= (2 * sizeof(void*))) ? 4 * sizeof(void*) : (align(size) + 2 * sizeof(void*));    	/* Align block size to satisfy size and header alignments */
    ptr = first_fit(size_a);	/* Looking for valid free block using first-fit method */
    if (ptr != NULL) 	/* A hit was found, then WRITE_w block with adjusted size into the free block */
    {
       allocate_block(ptr, size_a); /* Use internal helper routine to allocate the memory block */
       return ptr;
       //printf("%p ptr: \n", ptr);
    }
    else	/* if there is no fit, then request more memory and place the block */
    {
        /* Align the request to make sure it was 2-words aligned */

    	ptr = mem_request(size_a);
    	if (ptr != NULL)
    	{
    	    allocate_block(ptr, size_a);
    	    return ptr;
    	    //printf("%p ptr: \n", ptr);
    	}
    	else
    	{
    	    return NULL;
    	}
    }
}

/*
 * free
 */
void free(void* ptr)
{
	
if (ptr == NULL) // ensure the pointer is NOT Null before attempting to free block
{
	return;
}
    size_t size = READ_w_SIZE(Compute_HD(ptr));	/* Store header address of the printer that needs to be freed */
    WRITE_w(Compute_HD(ptr), Pack_w(size, 0)); 	/* Free the header pointer of the block*/
    WRITE_w(Compute_FT(ptr), Pack_w(size, 0)); 	/* Free the footer pointer of the block*/
    coalesce(ptr);
}
