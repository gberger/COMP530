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

    // If the return value of read is 0, it means the pipe was closed
    while(read(STDIN_FILENO, &c, sizeof(char)) > 0) {
        // Newline characters are passed down as space characters.
        if(c == '\n') {
            c = ' ';
        }

        write(STDOUT_FILENO, &c, sizeof(char));
    }

    return 0;
}