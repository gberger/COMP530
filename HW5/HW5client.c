/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

/* 
 * A client program that uses the remote shell service.
 *
 * This program reads input lines from stdin and sends them to
 * a server which interprets them as shell commands. The server
 * executes the commands, sending the output back to the client,
 * who finally sends it to stdout.
 *
 * The client has two positional parameters: (1) the DNS host name 
 * where the server program is running, and (2) the port number of 
 * the server's "welcoming" socket.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "Socket.h"
#include "HW5.h" /* definitions common to client and server */

/* Returns 1 if the given str consists entirely of
 * whitespace characters, as determined by `isspace`.
 */
int is_all_whitespace(char *str) {
  int i, len = strlen(str);
  for(i = 0; i < len; i++) {
    if(!isspace(str[i])) {
      return 0;
    }
  }
  return 1;
}

int main(int argc, char* argv[]) {
  int i, c, rc;
  int count = 0;

  char line_data[MAX_LINE]; 

  // variable to hold socket descriptor
  Socket connect_socket;

  if (argc < 3) {
    printf("No host and port\n");
    return -1;
  }

  /* connect to the server at the specified host and port;
   * blocks the process until the connection is accepted
   * by the server; creates a new data transfer socket.
   */
  connect_socket = Socket_new(argv[1], atoi(argv[2]));
  if (connect_socket < 0) {
    printf("Failed to connect to server\n");
    return -1;
  }

  // display the prompt
  printf("%% ");

  /* get a string from stdin up to and including 
   * a newline or to the maximim input line size.  
   * Continue getting strings from stdin until EOF.
   */ 
  while ((fgets(line_data, sizeof(line_data), stdin) != NULL)) {

    if(is_all_whitespace(line_data)) {
      // don't bother sending blank commands to server
      printf("%% ");
      continue;
    }

    // count includes '\0'
    count = strlen(line_data) + 1;

    // send the characters of the input line to the 
    // server using the data transfer socket.
    for (i = 0; i < count; i++) {
      c = line_data[i];
      rc = Socket_putc(c, connect_socket);
      if (rc == EOF) {
        // assume socket problem is fatal, end client
	      printf("Socket_putc EOF or error\n");             
        Socket_close(connect_socket);
        exit(-1);
       }
     }

    /* receive the converted characters for the string from
     * the server using the data transfer socket.  Expect the
     * server to return exactly the same number of characters
     * sent to it.
     */
    for (i = 0; i < MAX_LINE; i++) {
      c = Socket_getc(connect_socket);
      if (c == EOF) {
        printf("Socket_getc EOF or error\n");             
        Socket_close(connect_socket);
        exit(-1);
      } else {
        line_data[i] = c;
        if (line_data[i] == '\0') {
          break;
        }
      }
    }
    // be sure the string is terminated
    if (i == MAX_LINE) {
      line_data[i-1] = '\0';
    }

    // display the converted string on stdout
    printf("%s", line_data);

    // display the prompt
    printf("%% ");
  } 
  // end of while loop: at EOF

  Socket_close(connect_socket);
  exit(0);
}
