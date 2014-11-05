/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

// Standard includes
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Default line size
#define LINE_SIZE 80

// Array indexing consts for the pipes array
#define READ 0
#define WRITE 1

// Helper function, used to set a char array to zero
void clear_str(char* str, int size) {
    int i;
    for(i=0; i<size; i++) {
        str[i] = 0;
    }
}

// Data structure keeping three pairs of pipes
typedef struct {
    int *input_cr_pipes;
    int *cr_squash_pipes;
    int *squash_output_pipes;
} PipeCollection;

// Prototypes for functions constituting execution of each process.
void INPUT_func(void *void_pipes);
void CR_func(void *void_pipes);
void SQUASH_func(void *void_pipes);
void OUTPUT_func(void *void_pipes);

// Forks a process;
// On the child process, calls the function, passing the param, and exits after
// On the parent, just return
void fork_and_call(void (*fn)(void*), void *param) {
    pid_t pid = fork();

    if(pid == -1){
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if(pid == 0){
        fn(param);
        exit(EXIT_SUCCESS);
    }
    // else: parent, just return
}

/*
 * Create three child processes.
 * The four processes (children + father) execute following a 'pipe' pipeline:
 * 
 * [INPUT] ----------> [CR] -----------> [SQUASH] ---------------> [OUTPUT]
 *       (input_cr_pipes  (cr_squash_pipes)    (squash_output_pipes)
 *
 * INPUT reads characters from stdin and passes them to CR
 * CR transforms newline characters to space characters and passes them to SQUASH
 * SQUASH merges consecutive star characters into carets, and passes them to OUTPUT
 * OUTPUT outputs fixed-width lines to stdout
 * 
 * Execution stops after EOF
 */
int main (int argc, char const *argv[]) {

    // Setup the pipes
    int input_cr_pipes[2];
    int cr_squash_pipes[2];
    int squash_output_pipes[2];

    // Initialize PipeCollection structure
    PipeCollection pipes = {input_cr_pipes, cr_squash_pipes, squash_output_pipes};

    // Initialize pipes and check for errors
    if(pipe(input_cr_pipes) == -1
       || pipe(cr_squash_pipes) == -1
       || pipe(squash_output_pipes) == -1){
        perror("pipe creation failed");
        exit(EXIT_FAILURE);
    }

    // Start child processes
    fork_and_call(INPUT_func, (void*)(&pipes));
    fork_and_call(CR_func, (void*)(&pipes));
    fork_and_call(SQUASH_func, (void*)(&pipes));
    OUTPUT_func((void*)(&pipes));

    return 0;
}

// The execution path for process INPUT
void INPUT_func(void* void_pipes){
    // Cast into PipeCollection
    PipeCollection *pipes = (PipeCollection*)void_pipes;

    // Character that we process each time
    char c;

    // Save used pipe and close unused ones
    int output_fd = pipes->input_cr_pipes[WRITE];
    close(pipes->input_cr_pipes[READ]);
    close(pipes->cr_squash_pipes[READ]);
    close(pipes->cr_squash_pipes[WRITE]);
    close(pipes->squash_output_pipes[READ]);
    close(pipes->squash_output_pipes[WRITE]); 

    // This process loops while getting non-EOF chars from stdin.
    c = getchar();
    while(c != EOF) {
        write(output_fd, &c, sizeof(char));
        c = getchar();
    }

    // Close the pipe
    close(output_fd);
}

// The execution path for process CR
void CR_func(void* void_pipes){
    // Cast into PipeCollection
    PipeCollection *pipes = (PipeCollection*)void_pipes;

    // Character that we process each time
    char c;

    // Save used pipes and close unused ones
    int input_fd = pipes->input_cr_pipes[READ];
    int output_fd = pipes->cr_squash_pipes[WRITE];
    close(pipes->input_cr_pipes[WRITE]);
    close(pipes->cr_squash_pipes[READ]);
    close(pipes->squash_output_pipes[READ]);
    close(pipes->squash_output_pipes[WRITE]);

    // If the return value of read is 0, it means the pipe was closed
    while(read(input_fd, &c, sizeof(char)) > 0) {
        // Newline characters are passed down as space characters.
        if(c == '\n') {
            c = ' ';
        }

        write(output_fd, &c, sizeof(char));
    }

    // Close the pipes
    close(input_fd);
    close(output_fd);
}

// The execution path for process SQUASH
void SQUASH_func(void* void_pipes){
    // Cast into PipeCollection
    PipeCollection *pipes = (PipeCollection*)void_pipes;

    // Character that we process each time
    char c;
    int r;

    // Constants that we compare to and send to `write`
    char const caret = '^', star = '*';

    // Flag indicating with the character before the current is == star
    int last_was_star = 0;

    // Save used pipes and close unused ones
    int input_fd = pipes->cr_squash_pipes[READ];
    int output_fd = pipes->squash_output_pipes[WRITE];
    close(pipes->input_cr_pipes[READ]);
    close(pipes->input_cr_pipes[WRITE]);
    close(pipes->cr_squash_pipes[WRITE]);
    close(pipes->squash_output_pipes[READ]);

    // If the return value of read is 0, it means the pipe was closed
    while(read(input_fd, &c, sizeof(char)) > 0) {
        if(c == star) {
            // If the current character is a star...
            if(last_was_star == 1) {
                // And the last one was also a star, we pass down a caret.
                write(output_fd, &caret, sizeof(char));
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
                write(output_fd, &star, sizeof(char));
            }
            write(output_fd, &c, sizeof(char));
        }
    }

    // If the character before the pipe being closed was star,
    // we need to pass it down now, as we have retained it before.
    if(last_was_star) {
        write(output_fd, &star, sizeof(char));
    }

    // Close the pipes
    close(input_fd);
    close(output_fd);
}

// The execution path for process OUTPUT
void OUTPUT_func(void* void_pipes){
    // Cast into PipeCollection
    PipeCollection *pipes = (PipeCollection*)void_pipes;

    // Character that we process each time
    char c;

    // Save used pipe and close unused ones
    int input_fd = pipes->squash_output_pipes[READ];
    close(pipes->input_cr_pipes[READ]);
    close(pipes->input_cr_pipes[WRITE]);
    close(pipes->cr_squash_pipes[READ]);
    close(pipes->cr_squash_pipes[WRITE]);
    close(pipes->squash_output_pipes[WRITE]);

    // We keep a string with the length of the line, along with an index
    // variable indicating where the next character will be placed in
    // the string.
    char str[LINE_SIZE+1];
    int i = 0;

    clear_str(str, LINE_SIZE+1);

    // If the return value of read is 0, it means the pipe was closed
    while(read(input_fd, &c, sizeof(char)) > 0) {
        // We append each new character to a string.
        str[i++] = c;

        // After the string has reached (80) characters, we print it,
        // along with a newline, clear the string and reset the index counter.
        if(i == LINE_SIZE) {
            printf("%s\n", str);
            clear_str(str, LINE_SIZE+1);
            i = 0;
        }
    }

    // Close the pipe
    close(input_fd);
}
