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

    // This process loops while getting non-EOF chars from stdin.
    c = getchar();
    while(c != EOF) {
        write(STDOUT_FILENO, &c, sizeof(char));
        c = getchar();
    }
    
    return 0;
}
