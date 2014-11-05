/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

/* 
 * A server program that provides the service of a remote shell.
 * It is implemented using the "daemon-service" model
 * In this model, multiple clients can be serviced
 * concurrently by the service. The server main process is 
 * a simple loop that accepts incoming socket connections, and for 
 * each new connection established, uses fork() to create a child 
 * process that is a new instance of the service process.  This child 
 * process will provide the service for the single client program that 
 * established the socket connection.  
 *
 * Each new instance of the server process creates a temporary file
 * for itself. The instance then acts in a loop, accepting input lines
 * from its client. For each line, it forks a new process, which interprets
 * the line, parsing it into a program filename and its arguments.
 * This child then execs the given program with its arguments,
 * Meanwhile, the father is waiting for the child. Upon child termination,
 * the father reads the output that was saved to the temporary file,
 * and sends the output to the client, along with an informational 
 * line containing the exit status of the child and other stats.
 * The loop then continues.
 *
 * Since the main process (the daemon) is intended to be continuously
 * available, it has no defined termination condition and must be
 * terminated by an external action (Ctrl-C or kill command).
 *
 * The server has one parameter: the port number that will be used
 * for the "welcoming" socket where the client makes a connection.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Socket.h"
#include "HW5.h" /* definitions shared by client and server */

#define FILENAME_SIZE 64
#define INFOSTR_SIZE 256


void shell_service(Socket connect_socket);

int main(int argc, char* argv[]) {
  // variables to hold socket descriptors
  ServerSocket welcome_socket;
  Socket connect_socket;

  pid_t spid, term_pid;
  int chld_status;
  bool forever = true;

  if (argc < 2) {
    printf("No port specified\n");
    return -1;
  }

  // create a "welcoming" socket at the specified port
  welcome_socket = ServerSocket_new(atoi(argv[1]));
  if (welcome_socket < 0)
  {
    printf("Failed new server socket\n");
    return -1;
  }

  // The daemon infinite loop begins here; terminate with external action
  while (forever) {
    /* accept an incoming client connection; blocks the
     * process until a connection attempt by a client.
     * creates a new data transfer socket.
     */
    connect_socket = ServerSocket_accept(welcome_socket);

    if (connect_socket < 0) {
      printf("Failed accept on server socket\n");
      exit(-1);
    }

    // create a child that will act as the service instance for this client
    spid = fork();
    if (spid == -1) {
      perror("Failed on forking for a client"); 
      exit(-1);
    } else if (spid == 0) {
      // service process
      // we dont't need the welcome socket, close it
      Socket_close(welcome_socket);
      shell_service(connect_socket);
      Socket_close(connect_socket);
      exit(0);
    } else {
      // parent process; just close the socket, we don't need it
      Socket_close(connect_socket);
      term_pid = waitpid(-1, &chld_status, WNOHANG);
    }
  }
  // end of infinite loop for the daemon
}

void shell_service(Socket connect_socket) {
  int i, c, rc, len;
  bool forever = true;
  char input_data[MAX_LINE];
  char info_string[INFOSTR_SIZE];

  // setupd the temp file
  pid_t my_pid = getpid();
  char filename[FILENAME_SIZE] = {0};
  sprintf(filename, "tmp%d", my_pid);
  FILE *tmpfile;

  printf("New connection: %d\n", (int)connect_socket);
  printf("Filename: %s\n", filename);

  while (forever) { // actually, until EOF on connect socket
    // open the tempfile. Also erases all contents
    tmpfile = fopen(filename, "w");

    /* get a null-terminated string from the client on the data
     * transfer socket up to the maximim input line size. Continue 
     * getting strings from the client until EOF on the socket.
     */ 
    for (i = 0; i < MAX_LINE; i++) {
      c = Socket_getc(connect_socket);
      if (c == EOF) {
        // assume socket EOF ends service for this client
        printf("End connection: %d\n", (int)connect_socket); 
        return;
      } else {
        input_data[i] = c;
        if (c == '\0') {
          break;
        }
      }
    }

    // be sure the string is terminated if max size reached
    if (i == MAX_LINE) {
      input_data[i-1] = '\0';
    }

    // write to tempfile
    fprintf(tmpfile, "%s", input_data);
    fclose(tmpfile);

    // set the file back to start for reading
    tmpfile = fopen(filename, "r");

    // read from the file and send it back to the client
    while ((c = fgetc(tmpfile)) != EOF) {
      rc = Socket_putc(c, connect_socket);
      if (rc == EOF) {
        // assume socket EOF ends service for this client
        printf("Socket_putc EOF or error\n");
        return;
      }
    }

    sprintf(info_string, "Informational string. Blabla. Input had size of %d.\n", strlen(input_data));

    // +1 for including '\0'
    len = strlen(info_string) + 1;
    for(i = 0; i < len; i++) {
      rc = Socket_putc(info_string[i], connect_socket);
      if (rc == EOF) {
        // assume socket EOF ends service for this client
        printf("Socket_putc EOF or error\n");
        return;
      }
    }

    // close the tempfile
    fclose(tmpfile);
  } 
  // end while loop of the service process

  // remove the temporary file
  remove(filename);
}
