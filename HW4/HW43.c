/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

#include <unistd.h>
#include <stdio.h>

int main(int argc, char const *argv[]) {
    // Character that we process each time
    char c;

    // Constants that we compare to and send to `write`
    char const caret = '^', star = '*';

    // Flag indicating with the character before the current is == star
    int last_was_star = 0;

    // If the return value of read is 0, it means the pipe was closed
    while(read(STDIN_FILENO, &c, sizeof(char)) > 0) {
        if(c == star) {
            // If the current character is a star...
            if(last_was_star == 1) {
                // And the last one was also a star, we pass down a caret.
                write(STDOUT_FILENO, &caret, sizeof(char));
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
                write(STDOUT_FILENO, &star, sizeof(char));
            }
            write(STDOUT_FILENO, &c, sizeof(char));
        }
    }

    // If the character before the pipe being closed was star,
    // we need to pass it down now, as we have retained it before.
    if(last_was_star) {
        write(STDOUT_FILENO, &star, sizeof(char));
    }

    return 0;
}