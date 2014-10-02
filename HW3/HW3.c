// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Library and custom includes
#include "st.h"
#include "semaphore.h"
#include "buffer.h"

// Defaults
#define DEFAULT_INPUT_STREAM stdin
#define DEFAULT_OUTPUT_STREAM stdout
#define DEFAULT_BUFFER_SIZE 256
#define DEFAULT_LINE_NUMBER 5
#define DEFAULT_SLEEP_TIME 100000

// Initialization data structure for the input thread
typedef struct {
    FILE *input_stream;         // Input for the thread
    Buffer *outBuffer;          // Pointer to shared buffer for output
} InputThreadInit;

// Initialization data structure for each processing thread
typedef struct {
    Buffer *inBuffer;           // Pointer to shared buffer for input
    Buffer *outBuffer;          // Pointer to shared buffer for output
} ProcessingThreadInit;

// Initialization data structure for the output thread
typedef struct {
    Buffer *inBuffer;           // Pointer to shared buffer for input
    FILE *output_stream;        // Output target for the thread
} OutputThreadInit;


// Prototypes for functions constituting execution of each thread.
void *thread_INPUT_func(void *state);
void *thread_CR_func(void *state);
void *thread_SQUASH_func(void *state);
void *thread_OUTPUT_func(void *state);


// Helper function, used to set a char array to zero
void clear_str(char* str, int size) {
    int i;
    for(i=0; i<size; i++) {
        str[i] = 0;
    }
}

/*
 * Create four threads using the ST library.
 * The four threads execute following a buffer pipeline:
 * 
 * [INPUT] ----------> [CR] -----------> [SQUASH] ---------------> [OUTPUT]
 *       (bufferInputCR)  (bufferCRSquash)      (bufferSquashOutput)
 *
 * The input thread reads TODOOOOOOOOOOOOOOOOOO
 * 
 * Execution stops after a EOF is passed along the buffer pipeline.
 */
int main (int argc, char const *argv[]) {

    // Initialize the libST runtime.
    st_init();

    // Set the file descriptors
    FILE *input_stream = DEFAULT_INPUT_STREAM;
    FILE *output_stream = DEFAULT_OUTPUT_STREAM;

    // Create the buffers, as describred above
    Buffer* bufferInputCR      = buffer_create(DEFAULT_BUFFER_SIZE);
    Buffer* bufferCRSquash     = buffer_create(DEFAULT_BUFFER_SIZE);
    Buffer* bufferSquashOutput = buffer_create(DEFAULT_BUFFER_SIZE);

    // Create the initial state structure for the INPUT thread
    InputThreadInit inputState = {
        input_stream,
        bufferInputCR
    };

    // Create the initial state structure for the CR thread
    ProcessingThreadInit crState = {
        bufferInputCR,
        bufferCRSquash
    };

    // Create the initial state structure for the SQUASH thread
    ProcessingThreadInit squashState = {
        bufferCRSquash,
        bufferSquashOutput
    };

    // Create the initial state structure for the OUTPUT thread
    OutputThreadInit outputState = {
        bufferSquashOutput,
        output_stream
    };

    // Create thread INPUT
    if (st_thread_create(thread_INPUT_func, &inputState, 0, 0) == NULL) {
        perror("st_thread_create failed for thread INPUT");
        exit(EXIT_FAILURE);
    }

    // Create thread CR
    if (st_thread_create(thread_CR_func, &crState, 0, 0) == NULL) {
        perror("st_thread_create failed for thread CR");
        exit(EXIT_FAILURE);
    }

    // Create thread SQUASH
    if (st_thread_create(thread_SQUASH_func, &squashState, 0, 0) == NULL) {
        perror("st_thread_create failed for thread SQUASH");
        exit(EXIT_FAILURE);
    }

    // Create thread OUTPUT
    if (st_thread_create(thread_OUTPUT_func, &outputState, 0, 0) == NULL) {
        perror("st_thread_create failed for thread OUTPUT");
        exit(EXIT_FAILURE);
    }

    // Exit from main via ST.
    st_thread_exit(NULL);
    return 0;

}

// The execution path for thread INTPUT
void *thread_INPUT_func(void *state) {
    // Appease the compiler by casting the input
    InputThreadInit *init = state;
    char c;

    while(1) {
        c = getchar();
        buffer_deposit(init->outBuffer, c);
        st_usleep(DEFAULT_SLEEP_TIME);
    }

    st_thread_exit(NULL);
}

// The execution path for thread CR
void *thread_CR_func(void *state) {
    // Appease the compiler by casting the input
    ProcessingThreadInit *init = state;
    char c;

    while(1) {
        c = buffer_remove(init->inBuffer);

        if(c == '\n') {
            c = ' ';
        }

        buffer_deposit(init->outBuffer, c);
    }

    st_thread_exit(NULL);
}

// The execution path for thread SQUASH
void *thread_SQUASH_func(void *state) {
    // Appease the compiler by casting the input
    ProcessingThreadInit *init = state;
    char c;
    int last_was_star = 0;

    while(1) {
        c = buffer_remove(init->inBuffer);

        if(c == '*') {
            if(last_was_star == 1) {
                buffer_deposit(init->outBuffer, '^');
                last_was_star = 0;
            } else {
                last_was_star = 1;
            }
        } else {
            if(last_was_star) {
                last_was_star = 0;
                buffer_deposit(init->outBuffer, '*');
            }
            buffer_deposit(init->outBuffer, c);   
        }

    }

    st_thread_exit(NULL);
}

// The execution path for thread OUTPUT
void *thread_OUTPUT_func(void *state) {
    // Appease the compiler by casting the input
    OutputThreadInit *init = state;
    char c;
    char str[DEFAULT_LINE_NUMBER+1];
    int i = 0;

    clear_str(str, DEFAULT_LINE_NUMBER+1);

    while(1) {
        c = buffer_remove(init->inBuffer);
        str[i++] = c;

        if(i == DEFAULT_LINE_NUMBER) {
            printf("%s\n", str);
            clear_str(str, DEFAULT_LINE_NUMBER+1);
            i = 0;
        }
    }

    st_thread_exit(NULL);
}
