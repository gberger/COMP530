/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

/* REFERENCES USED
	http://www.csc.villanova.edu/~mdamian/threads/posixsem.html#init
	http://pubs.opengroup.org/onlinepubs/009695399/basedefs/semaphore.h.html
	https://code.google.com/p/torsocks/issues/detail?id=2
*/

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // fork
#include <sys/types.h> // kill
#include <signal.h>    // kill

// Library and custom includes
#include "buffer.h"

// Defaults
#define DEFAULT_INPUT_STREAM stdin
#define DEFAULT_OUTPUT_STREAM stdout
#define DEFAULT_LINE_NUMBER 80
#define ERROR -1

// Initialization data structure for the input process
typedef struct {
	FILE *input_stream;         // Input for the process
	Buffer *outBuffer;          // Pointer to shared buffer for output
} InputProcessInit;

// Initialization data structure for each processing process
typedef struct {
	Buffer *inBuffer;           // Pointer to shared buffer for input
	Buffer *outBuffer;          // Pointer to shared buffer for output
} ProcessingProcessInit;

// Initialization data structure for the output process
typedef struct {
	Buffer *inBuffer;           // Pointer to shared buffer for input
	FILE *output_stream;        // Output target for the process
} OutputProcessInit;


// Prototypes for functions constituting execution of each process.
void child_INPUT_func(void *state);
void child_CR_func(void *state);
void child_SQUASH_func(void *state);
void child_OUTPUT_func(void *state);

pid_t forkChild(void (*fn)(void *), void* state);
void waitForChildren(pid_t* childpids, int num_children);


/*
 * Create four child processes.
 * They execute following a 'pipe' pipeline:
 * 
 * [INPUT] -------------> [CR] -----------> [SQUASH] ---------------> [OUTPUT]
 *       (input_cr_pipes)   (cr_squash_pipes)      (squash_output_pipes)
 *
 * INPUT reads characters from stdin and passes them to CR
 * CR transforms newline characters to space characters and passes them to SQUASH
 * SQUASH merges consecutive star characters into carets, and passes them to OUTPUT
 * OUTPUT outputs fixed-width lines to stdout
 * 
 * Execution stops after EOF
 */
int main (int argc, char const *argv[]) {

	// Set the file descriptors
	FILE *input_stream = DEFAULT_INPUT_STREAM;
	FILE *output_stream = DEFAULT_OUTPUT_STREAM;

	// Create the buffers, as describred above
	Buffer* bufferInputCR      = buffer_create();
	Buffer* bufferCRSquash     = buffer_create();
	Buffer* bufferSquashOutput = buffer_create();

	// Create the initial state structure for the INPUT Process
	InputProcessInit inputState = {
		input_stream,
		bufferInputCR
	};

	// Create the initial state structure for the CR Process
	ProcessingProcessInit crState = {
		bufferInputCR,
		bufferCRSquash
	};

	// Create the initial state structure for the SQUASH Process
	ProcessingProcessInit squashState = {
		bufferCRSquash,
		bufferSquashOutput
	};

	// Create the initial state structure for the OUTPUT Process
	OutputProcessInit outputState = {
		bufferSquashOutput,
		output_stream
	};

	pid_t child_pids[4];

	// Create process INPUT
	child_pids[0] = forkChild(child_INPUT_func, &inputState);
	// Create process CR
	child_pids[1] = forkChild(child_CR_func, &crState);
	// Create process SQUASH
	child_pids[2] = forkChild(child_SQUASH_func, &squashState);
	// Create process OUTPUT
	child_pids[3] = forkChild(child_OUTPUT_func, &outputState);

    //wait for them
    waitForChildren(child_pids, 4);

    //cleanup
    buffer_free(bufferInputCR);
    buffer_free(bufferCRSquash);
    buffer_free(bufferSquashOutput);

	return 0;
}


pid_t forkChild(void (*fn)(void *), void* state){
	//This function takes a pointer to a function as an argument
	//and the functions argument. It then returns the forked child's pid.

	pid_t child_pid;
	switch (child_pid = fork()) {
		case ERROR:
			perror("fork error");
			exit(EXIT_FAILURE);
		case 0: 
			(*fn)(state);
			exit(EXIT_FAILURE); //error if we return from function
		default:
			return child_pid;
	}
}


void waitForChildren(pid_t* child_pids, int num_children){
	int status, i;
	while(ERROR < wait(&status)){ //Here the parent waits on any child.
		if(!WIFEXITED(status)){ //If the child terminated unsuccessfully, kill all children.
			for(i = 0; i < num_children; i++) {
				kill(child_pids[i], SIGKILL);
			}
			break;
		}
	}
}


// The execution path for process INTPUT
void child_INPUT_func(void *state) {
	// Appease the compiler by casting the input
	InputProcessInit *init = state;
	
	// Character that we process each time
	char c;

	// This process loops while getting characters from stdin.
	while(1) {
		c = getchar();
		buffer_deposit(init->outBuffer, c);

		// If we reach EOF, we break the while loop
		if(c == EOF) {
			break;
		}
	}

	// This is only reached after EOF.
	exit(0);
}

// The execution path for process CR
void child_CR_func(void *state) {
	// Appease the compiler by casting the input
	ProcessingProcessInit *init = state;
	
	// Character that we process each time
	char c;

	// This process loops while getting characters from a buffer.
	while(1) {
		c = buffer_remove(init->inBuffer);

		// Newline characters are passed down as space characters.
		if(c == '\n') {
			c = ' ';
		}

		buffer_deposit(init->outBuffer, c);

		// If we reach EOF, we break the while loop
		if(c == EOF) {
			break;
		}
	}

	// This is only reached after EOF.
	exit(0);
}

// The execution path for process SQUASH
void child_SQUASH_func(void *state) {
	// Appease the compiler by casting the input
	ProcessingProcessInit *init = state;
	
	// Character that we process each time
	char c;

	// Flag indicating with the character before the current is == '*'
	int last_was_star = 0;

	// This process loops while getting characters from a buffer.
	while(1) {
		c = buffer_remove(init->inBuffer);

		if(c == '*') {
			// If the current character is a star...
			if(last_was_star == 1) {
				// And the last one was also a star, we pass down a caret.
				buffer_deposit(init->outBuffer, '^');
				last_was_star = 0;
			} else {
				// Otherwise, we refrain from passing down the star for now.
				last_was_star = 1;
			}
		} else {
			// If the current character is not a star, but the last was,
			// we have to reset the flag and pass down the star before
			// passing down the current character.
			if(last_was_star) {
				last_was_star = 0;
				buffer_deposit(init->outBuffer, '*');
			}
			buffer_deposit(init->outBuffer, c);   
		}

		// If we reach EOF, we break the while loop
		if(c == EOF) {
			break;
		}
	}

	// This is only reached after EOF.
	exit(0);
}

// The execution path for process OUTPUT
void child_OUTPUT_func(void *state) {
	// Appease the compiler by casting the input
	OutputProcessInit *init = state;
	
	// Character that we process each time
	char c;

	// We keep a string with the length of the line, along with an index
	// variable indicating where the next character will be placed in
	// the string.
	char str[DEFAULT_LINE_NUMBER+1];
	int i = 0;

	memset(str, 0, sizeof(str));

	// This process loops while getting characters from a buffer.
	while(1) {
		c = buffer_remove(init->inBuffer);

		// If we reach EOF, we break the while loop
		if(c == EOF) {
			break;
		}

		// We append each new character to a string.
		str[i++] = c;

		// After the string has reached (80) characters, we print it,
		// along with a newline, clear the string and reset the index counter.
		if(i == DEFAULT_LINE_NUMBER) {
			printf("%s\n", str);
			memset(str, DEFAULT_LINE_NUMBER+1, '\0');
			i = 0;
		}
	}

	// This is only reached after EOF.
	exit(0);
}
