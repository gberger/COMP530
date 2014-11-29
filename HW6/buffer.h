/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

#include <semaphore.h>

#define BUFFER_SIZE 256

typedef struct {
	char arr[BUFFER_SIZE];
	int nextIn;
	int nextOut;
	sem_t *emptySemaphore;
	sem_t *fullSemaphore;
} Buffer;

// Creates a new bounded, ST-asynchronous buffer
// that is able to store `size` characters.
Buffer* buffer_create();

// Removes the oldest character from the buffer and returns it.
// When asked to remove from an empty buffer, acts asynchronously.
char buffer_remove(Buffer *b);

// Puts a character in the buffer.
// When asked to deposit to a full buffer, acts asynchronously.
void buffer_deposit(Buffer *b, char c);

// Frees the memory allocated by the buffer and its substructures.
void buffer_free(Buffer *b);
