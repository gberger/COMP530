/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

#include <stdlib.h>
#include "st.h"
#include "semaphore.h"
#include "buffer.h"


// Creates a new bounded, ST-asynchronous buffer
// that is able to store `size` characters.
Buffer *buffer_create(int size) {
	Buffer *b = malloc(sizeof(Buffer));

	// These semaphores guarantee asynchronous, non-intrusive execution
	// when depositing to a full buffer or removing from an empty buffer.
	semaphore *empty = malloc(sizeof(semaphore));
	semaphore *full = malloc(sizeof(semaphore));

    createSem(empty, size);
    createSem(full, 0);

	b->size = size;
	b->nextIn = 0;
	b->nextOut = 0;
	b->arr = malloc(sizeof(char) * size);
	b->emptySemaphore = empty;
	b->fullSemaphore = full;

	if (b->arr == NULL) {
        perror("create_buffer failed to allocate memory for the buffer");
        exit(EXIT_FAILURE);
    }

	return b;
}

// Removes the oldest character from the buffer and returns it.
// When asked to remove from an empty buffer, acts asynchronously.
char buffer_remove(Buffer *b) {
	// Standard buffer removal implementation from slides.
	char c;

	down(b->fullSemaphore);
	c = (b->arr)[b->nextOut];
	b->nextOut = (b->nextOut + 1) % b->size;
	up(b->emptySemaphore);

	return c;
}

// Puts a character in the buffer.
// When asked to deposit to a full buffer, acts asynchronously.
void buffer_deposit(Buffer *b, char c) {
	// Standard buffer deposit implementation from slides.
	down(b->emptySemaphore);
	(b->arr)[b->nextIn] = c;
	b->nextIn = (b->nextIn + 1) % b->size;
	up(b->fullSemaphore);
}

// Frees the memory allocated by the buffer and its substructures.
void buffer_free(Buffer *b) {
	free(b->arr);
	free(b->emptySemaphore);
	free(b->fullSemaphore);
	free(b);
}
