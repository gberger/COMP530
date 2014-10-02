#include <stdlib.h>
#include "st.h"
#include "semaphore.h"
#include "buffer.h"


Buffer *buffer_create(int size) {
	Buffer *b = malloc(sizeof(Buffer));

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

char buffer_remove(Buffer *b) {
	char c;

	down(b->fullSemaphore);
	c = (b->arr)[b->nextOut];
	b->nextOut = (b->nextOut + 1) % b->size;
	up(b->emptySemaphore);

	return c;
}

void buffer_deposit(Buffer *b, char c) {
	down(b->emptySemaphore);
	(b->arr)[b->nextIn] = c;
	b->nextIn = (b->nextIn + 1) % b->size;
	up(b->fullSemaphore);

	return;
}

void buffer_free(Buffer *b) {
	free(b->arr);
	free(b->emptySemaphore);
	free(b->fullSemaphore);
	free(b);

	return;
}
