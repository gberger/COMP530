/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "buffer.h"

// Fix for OSX
#ifndef  MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define FLAG_SHARED 1
#define ERROR -1

// General helper function for creating shared, anonymous
// memory maps of size `size`. 
static void *create_mmap(size_t size) {
	//These are the neccessary arguments for mmap. See man mmap.
	void* start_addr = 0; // let the system decide where to place it
	int protections = PROT_READ|PROT_WRITE; //can read and write
	int flags = MAP_SHARED|MAP_ANONYMOUS; //it is shared between processes and does not map to a file.
	int fd = -1; //We could make it map to a file as well but here it is not needed.
	off_t offset = 0;
	
	//Create memory map
	void* addr = mmap(start_addr, size, protections, flags, fd, offset);
	
	//on an error mmap returns void* -1
	if ((void*) ERROR == addr){
		perror("error with mmap");
		exit(EXIT_FAILURE);
	}
	return addr;
}

// Creates a memory map for a Buffer
static Buffer* create_mmap_for_buffer(){
	return (Buffer*)create_mmap(sizeof(Buffer));
}

// Creates a memory map for a sem_t
static sem_t *create_mmap_for_semaphore() {
	return (sem_t*)create_mmap(sizeof(sem_t));
}

// General helper function for destroying memory maps.
static void delete_mmap(void* addr, size_t size) {
	//This deletes the memory map at given address. see man mmap
	if (ERROR == munmap(addr, size)){
		perror("error deleting mmap");
		exit(EXIT_FAILURE);
	}	
}

// Deletes a memory map for a Buffer
static void delete_mmap_for_buffer(Buffer* addr){
	delete_mmap((void*)addr, sizeof(Buffer));
}

// Deletes a memory map for a sem_t
static void delete_mmap_for_semaphore(sem_t* addr) {
	delete_mmap((void*)addr, sizeof(sem_t));
}


// Creates a new bounded, asynchronous buffer
// that is able to store `BUFFER_SIZE` characters.
// This buffer can be shared between threads.
Buffer *buffer_create() {
	// Instead of using malloc, we will use mmap to create the Buffer
	// and the semaphores. 
	Buffer *b = create_mmap_for_buffer();

	// These semaphores guarantee asynchronous, non-intrusive execution
	// when depositing to a full buffer or removing from an empty buffer.
	sem_t *empty = create_mmap_for_semaphore();
	sem_t *full  = create_mmap_for_semaphore();

	sem_init(empty, FLAG_SHARED, BUFFER_SIZE);
	sem_init(full, FLAG_SHARED, 0);

	b->nextIn = 0;
	b->nextOut = 0;
	b->emptySemaphore = empty;
	b->fullSemaphore = full;

	return b;
}

// Removes the oldest character from the buffer and returns it.
// When asked to remove from an empty buffer, acts asynchronously.
char buffer_remove(Buffer *b) {
	// Standard buffer removal implementation from slides.
	char c;

	sem_wait(b->fullSemaphore);
	c = (b->arr)[b->nextOut];
	b->nextOut = (b->nextOut + 1) % BUFFER_SIZE;
	sem_post(b->emptySemaphore);

	return c;
}

// Puts a character in the buffer.
// When asked to deposit to a full buffer, acts asynchronously.
void buffer_deposit(Buffer *b, char c) {
	// Standard buffer deposit implementation from slides.
	sem_wait(b->emptySemaphore);
	(b->arr)[b->nextIn] = c;
	b->nextIn = (b->nextIn + 1) % BUFFER_SIZE;
	sem_post(b->fullSemaphore);
}

// Frees the memory allocated by the buffer and its substructures.
void buffer_free(Buffer *b) {
	delete_mmap_for_semaphore(b->emptySemaphore);
	delete_mmap_for_semaphore(b->fullSemaphore);
	delete_mmap_for_buffer(b);
}
