/*
 * kernel_code.c - Project 2, CMPSC 473
 * Copyright 2023 Ruslan Nikolaev <rnikola@psu.edu>
 * Distribution, modification, or usage without explicit author's permission
 * is not allowed.
 */

#include <kernel.h>
#include <types.h>
#include <printf.h>
#include <malloc.h>
#include <string.h>

void *page_table = NULL; /* TODO: Must be initialized to the page table address */
void *user_stack = NULL; /* TODO: Must be initialized to a user stack virtual address */
void *user_program = NULL; /* TODO: Must be initialized to a user program virtual address */
void kernel_init(void *ustack, void *uprogram, void *memory, size_t memorySize)
{
	/* Q3 */
	//printf("%Memory Addr: %p\n", memory);
	uint64_t *pte = memory; // Initialize start of pte at start of usable memory
	//printf("Start of the pte: %l\n", pte);
	uint64_t *pde = pte + 1048576; // Initialize start of pde at end of pte
	//printf("Start of pde: %p\n", pde);
	uint64_t *pdpe = pde + 2048; // Initialize start of pdpe at end of pde
	//printf("Start of pdpe: %p\n", pdpe);
	uint64_t *pmle4e = pdpe + 512 ; // Initialize start of pmle4e at end of pdpe
	//printf("Start of pmle4e: %p\n", pmle4e);
	//pte[0] = (uint64_t) 0;
	
	for (size_t i = 0; i < 1048576; i++) {
	// For 1 << 32 total physical memory address, store 1 << 20 entries in respective locations of pte
		pte[i] = (uint64_t) (i * 4096) | (0x3);
		//printf("%p\n", *pte);
	}
	
	//printf("pte 0 %p\n", pte[0]);
	//printf("pte 1 %p\n", pte[1]);
	//printf("pte 2 %p\n", pte[2]);
	//printf("pte 1048574 %p\n", pte[1048574]);
	//printf("pte 1048575 %p\n", pte[1048575]);
	//printf("pte 1048576 %p\n", pte[1048576]);
	//printf("pte %p\n", pte);
	for (size_t i = 0; i < 2048; i++) {
	// For 1 << 20 total pte pages, store 1 << 11 entries in respective locations of pde
		pde[i] = (uint64_t) (pte + (i * 512)) | (0x3);
		//printf("%p\n", *pte);
	}
	
	/* for (size_t i = 0; i < 10; i++) {
		printf("pde%p\n", pde[i]);
	} */
	for (size_t i = 0; i < 4; i++) {
	// For 1 << 11 total pde pages, store 4 entries in respective locations of pdpe
		pdpe[i] = (uint64_t) (pde + (i * 512))| (0x3);
	}
	
	// For 4 total pdpe pages, store 1 entry in respective locations of pmle4e
	for (size_t i = 0; i < 512; i++) {
		pmle4e[i] = (uint64_t) 0;
	}
	pmle4e[0] = (uint64_t) (pdpe) | (0x3);
	//printf("pmle4e[0]: %p\n", pmle4e[0]);
	/* Q4 */
	// Initialize start address for user space level 3 page tables
	uint64_t *user_pte = pmle4e + 4096; // Initialize start of user space pte after the end of pmle4e + 4096
	uint64_t *user_pde = user_pte + 262144; // Initialize start of user space pde at end of user_pte + 512 * 512
	uint64_t *user_pdpe = user_pde + 262144; // Initialize start of user space pdpe at end of user_pde + 512 * 512
	for (size_t i = 0; i < 512; i++) {
	// Initialize all entries in user_pte to 0
		user_pte[i] = (uint64_t) 0;
	}
	// Set user_pte entry 510 to address of  top of ustack
	user_pte[510] = (uint64_t) (ustack - 4096) | (0x7);
	// Set user_pte entry 511 to address of uprogram
	user_pte[511] = (uint64_t) (uprogram) | (0x7);
	//printf("pte[510]: %p\n", user_pte[510]);
	//printf("pte[511]: %p\n", user_pte[511]);
	for (size_t i = 0; i < 512; i++) {
	// Initialize all entries in user_pde to 0
		user_pde[i] = (uint64_t) 0;
	}
	// Set user_pde entry 511 to address of user_pte
	user_pde[511] = (uint64_t) (user_pte) | (0x7);
	for (size_t i = 0; i < 512; i++) {
	// Initialize all entries in user_pdpe to 0
		user_pdpe[i] = (uint64_t) 0;
	}
	// Set user_pdpe entry 511 to address of user_pde
	user_pdpe[511] = (uint64_t) (user_pde) | (0x7);
	// Set pmle4e entry 511 to address of user_pdpe
	pmle4e[511] = (uint64_t) (user_pdpe) | (0x7);
	//printf("pmle4e[511]: %p\n", pmle4e[511]);
	
	// 'memory' points to the place where memory can be used to create
	// page tables (assume 1:1 initial virtual-to-physical mappings here)
	// 'memorySize' is the maximum allowed size, do not exceed that (given just in case)
	// TODO: Create a page table here
	// Use address of pmle4e as address of page table
	page_table = pmle4e;
	
	// TODO: It is OK for Q1-Q3, but should changed
	// for the final part's Q4 when creating your own user page table
	// Set addrress of ustack to VA of respective 4 level offset, -4096 to make it point to top of ustack
	user_stack = (uint64_t *) 0xfffffffffffff000 - (4096LL);
	//user_stack = ustack;
	// TODO: It is OK for Q1-Q3, but should changed
	// for the final part's Q4 when creating your own user page table
	// Set addrress of uprogram to VA of respective 4 level offset
	user_program = (uint64_t *) 0xfffffffffffff000;
	//user_program = uprogram;
	// The remaining portion just loads the page table,
	// this does not need to be changed:
	// load 'page_table' into the CR3 register
	const char *err = load_page_table(page_table);
	if (err != NULL) {
		printf("ERROR: %s\n", err);
	}

	// The extra credit assignment
	mem_extra_test();
}

/* The entry point for all system calls */
long syscall_entry(long n, long a1, long a2, long a3, long a4, long a5)
{
	// TODO: the system call handler to print a message (n = 1)
	// the system call number is in 'n', make sure it is valid!

	// Arguments are passed in a1,.., a5 and can be of any type
	// (including pointers, which are casted to 'long')
	// For the project, we only use a1 which will contain the address
	// of a string, cast it to a pointer appropriately 

	// For simplicity, assume that the address supplied by the
	// user program is correct
	//
	// Hint: see how 'printf' is used above, you want something very
	// similar here
	if (n == 1 && a1 != NULL) { /* Check if the syscall num is correct and if a1 is valid */
		char* msg = (char*)a1; /* Cast a1 into char pointer for printf */
		printf("%s\n", msg); /* Print message needed */
		return 0;
	} else {
		return -1; /* Failure: -1 */
	}
}
