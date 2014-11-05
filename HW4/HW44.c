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

// Default line size
#define LINE_SIZE 80

// Helper function, used to set a char array to zero
void clear_str(char* str, int size) {
    int i;
    for(i=0; i<size; i++) {
        str[i] = 0;
    }
}

int main(int argc, char const *argv[]) {
    // Character that we process each time
    char c;

    // We keep a string with the length of the line, along with an index
    // variable indicating where the next character will be placed in
    // the string.
    char str[LINE_SIZE+1];
    int i = 0;

    clear_str(str, LINE_SIZE+1);

    // If the return value of read is 0, it means the pipe was closed
    while(read(STDIN_FILENO, &c, sizeof(char)) > 0) {
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

    return 0;
}