/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

#include <assert.h>
#include "st.h"
#include "semaphore.h"

// Decrement the semaphore value.
// If the value is 0, wait until it becomes greater than 0.
void down(semaphore *s){
	// If the value is 0, wait on the ST condition variable.
	// We don't use while because signal only wakes up one thread:
	//   http://state-threads.sourceforge.net/docs/reference.html#cond_signal
	if (s->value == 0) {
		st_cond_wait(s->sem_queue);
	} 

	// Assert that we are really with a value greater than zero.
	assert(s->value > 0);

	// Finally, decrement the value.
	(s->value)--; 
}

// increase the semaphore value
// signal other candiate process to resume
void up(semaphore *s){
	// Increment the value.
	(s->value)++;

	// Assert that we have a value greater than zero. (Overflows!)
	assert(s->value>0);

	// Signal 
	st_cond_signal(s->sem_queue);
}

// Creates a new semaphore with the given value. 
void createSem(semaphore *s, int value){
	// We assume the pointer points to an already allocated memory space.

	// The value must be greater than zero.
	assert(value >= 0);

	// Set the initial semaphore value.
	s->value = value;

	// Assign a new ST condition variable to the semaphore.
	s->sem_queue = st_cond_new();

	// st_cond_new may return NULL...
	if(s->sem_queue == NULL) {
        perror("createSem failed to create a condition variable");
        exit(EXIT_FAILURE);
	}
}
