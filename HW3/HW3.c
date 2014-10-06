/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

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
#define DEFAULT_LINE_NUMBER 80
#define DEFAULT_SLEEP_TIME 10000

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
    
    // Character that we process each time
    char c;

    // This thread loops while getting characters from stdin.
    while(1) {
        c = getchar();
        buffer_deposit(init->outBuffer, c);

        // If we reach EOF, we break the while loop
        if(c == EOF) {
            break;
        }

        // Because getchar is blocking, we need to sleep for a while
        // to make sure the other threads get their chance to run.
        st_usleep(DEFAULT_SLEEP_TIME);
    }

    // This is only reached after EOF; thus, the thread exits after EOF.
    st_thread_exit(NULL);
}

// The execution path for thread CR
void *thread_CR_func(void *state) {
    // Appease the compiler by casting the input
    ProcessingThreadInit *init = state;
    
    // Character that we process each time
    char c;

    // This thread loops while getting characters from a buffer.
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
    // Thus, the thread cleans up and exits after EOF.
    buffer_free(init->inBuffer);
    st_thread_exit(NULL);
}

// The execution path for thread SQUASH
void *thread_SQUASH_func(void *state) {
    // Appease the compiler by casting the input
    ProcessingThreadInit *init = state;
    
    // Character that we process each time
    char c;

    // Flag indicating with the character before the current is == '*'
    int last_was_star = 0;

    // This thread loops while getting characters from a buffer.
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
    // Thus, the thread cleans up and exits after EOF.
    buffer_free(init->inBuffer);
    st_thread_exit(NULL);
}

// The execution path for thread OUTPUT
void *thread_OUTPUT_func(void *state) {
    // Appease the compiler by casting the input
    OutputThreadInit *init = state;
    
    // Character that we process each time
    char c;

    // We keep a string with the length of the line, along with an index
    // variable indicating where the next character will be placed in
    // the string.
    char str[DEFAULT_LINE_NUMBER+1];
    int i = 0;

    clear_str(str, DEFAULT_LINE_NUMBER+1);

    // This thread loops while getting characters from a buffer.
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
            clear_str(str, DEFAULT_LINE_NUMBER+1);
            i = 0;
        }
    }

    
    // This is only reached after EOF. 
    // Thus, the thread cleans up and exits after EOF.
    buffer_free(init->inBuffer);
    st_thread_exit(NULL);
}
