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
#include <sys/stat.h>

#include "Socket.h"
#include "HW5.h" /* definitions shared by client and server */

#define FILENAME_SIZE 64
#define INFOSTR_SIZE 256


void shell_service(Socket connect_socket);


/*
  Splits a given string `str` when any of the
  separators present in the string `sep` occur.
  The return is the address of a dynamically
  allocated char* array, where the last entry
  will be a pointer to NULL. Each char* in the
  array is a word, as defined by the separator.
  Remember to free after done.
*/
char **split_separator(char *str, char *sep) {
  char **res = NULL;
  char *p;
  int n_spaces = 0;

  // Refer to `strtok` documentation for details on this
  p = strtok(str, sep);
  while(p) {
    n_spaces++;
    res = realloc(res, sizeof(char*) * n_spaces);

    if(res == NULL){
      fprintf(stderr, "Error while allocating!\n");
      exit(1);
    }

    res[n_spaces-1] = p;
    p = strtok(NULL, sep);
  }

  // Extend one more time, for the final NULL pointer
  res = realloc(res, sizeof(char*) * (n_spaces+1));

  if(res == NULL){
    fprintf(stderr, "Error while allocating!\n");
    exit(1);
  }

  res[n_spaces] = NULL;

  return res;
}

/*
  Checks if a given path is a program executable by the current
  user, according to `stat`.
  Returns 1 (true) or 0 (false).
*/
int check_executable(char *path) {
  // Details/source: http://stackoverflow.com/a/13098645
  struct stat sb;
  return stat(path, &sb) == 0 && sb.st_mode & S_IXUSR;
}

/*
  Returns an array of strings, each one being a 
  directory contained in the PATH env variable.
  Remember to free.
*/
char **get_paths() {
  char *path = getenv("PATH");

  if(path == NULL){
    fprintf(stderr, "Error while getting env PATH!\n");
    exit(1);
  }

  char **paths = split_separator(path, ":");
  return paths;
}

/*
  Given a program name `cmd` (such as "ls"), and
  given `paths`, the result of `get_paths`, checks if any of these
  directories enumerated in `paths` contains a file named `cmd` that
  is executable (as defined by `check_executable`).
  If `cmd` starts with a '/', it's an absolute path. Return it.
  If there is a file in the current working directory that has the
  same name as `cmd` and is executable, its full path is returned.
  If a match is found, the full path of the executable is returned.
  Otherwise, NULL is returned.
 */
char *search_path(char *cmd, char **paths){
  char *cwd = getenv("PWD");
  char *possible;

  if(cwd == NULL){
    fprintf(stderr, "Error while getting env PWD!\n");
    exit(1);
  }

  // First, we if it starts with a slash, it is an absolute path. Return it.
  if(cmd[0] == '/') {
    possible = malloc(sizeof(char) * (strlen(cmd)+1));
    if(possible == NULL) {
      fprintf(stderr, "Error while allocating!\n");
      exit(1);
    }
    strcpy(possible, cmd);
    return possible;
  }

  // Then, we check the cwd.
  possible = (char*)malloc(sizeof(char) * (strlen(cwd) + 1 + strlen(cmd) + 1));
  if(possible == NULL){
    fprintf(stderr, "Error while allocating!\n");
    exit(1);
  }
  sprintf(possible, "%s/%s", cwd, cmd);
  if(check_executable(possible)){
    return possible;
  }

  // Finally, try each path.
  while(*paths != NULL){
    possible = (char*)realloc(possible, (sizeof(char)) * (strlen(*paths) + 1 + strlen(cmd) + 1));

    if(possible == NULL){
      fprintf(stderr, "Error while allocating!\n");
      exit(1);
    }

    sprintf(possible, "%s/%s", *paths, cmd);

    if(check_executable(possible)) {
      return possible;
    }

    paths++;
  }

  free(possible);

  // If no match, return NULL.
  return NULL;
}


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

  // stuff necessary for forking and execing
  pid_t fork_pid, term_pid;
  char **argv = NULL;
  char **paths = get_paths();
  char *exec_path;
  int status;
  char exit_status;

  // setupd the temp file
  pid_t my_pid = getpid();
  char filename[FILENAME_SIZE] = {0};
  sprintf(filename, "tmp%d", my_pid);
  FILE *tmpfile;

  printf("New connection: %d\n", (int)connect_socket);
  printf("Filename: %s\n", filename);

  while (forever) { // actually, until EOF on connect socket

    /* get a null-terminated string from the client on the data
     * transfer socket up to the maximim input line size. Continue 
     * getting strings from the client until EOF on the socket.
     */ 
    for (i = 0; i < MAX_LINE; i++) {
      c = Socket_getc(connect_socket);
      if (c == EOF) {
        // assume socket EOF ends service for this client
        printf("End connection: %d\n", (int)connect_socket);
        remove(filename);
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

    // Fork a new child process to parse the command line
    // and execute it, redirecting stdout to the tmpfile
    fork_pid = fork();

    if(fork_pid < 0) {
      perror("Failed on forking for a command"); 
      exit(-1);
    } else if(fork_pid == 0) {
      // child, will execute the command

      // redirect stdout contents to this file
      tmpfile = freopen(filename, "w", stdout);

      if(tmpfile == NULL){
        perror("freopen failed!");
        exit(-1);
      }

      // parse command line contents
      argv = split_separator(input_data, " \t\n\v\f\r");
      exec_path = search_path(argv[0], paths);

      if(exec_path != NULL) {
        execv(exec_path, argv);
        // if we are here, an error has occurred
        fprintf(stderr, "Error while executing!\n");
      } else {
        printf("%s: command not found\n", argv[0]);
      }
      free(argv);
      free(exec_path);
      fclose(tmpfile);
      exit(-1);
    } else {
      // parent, will wait for the child
      term_pid = waitpid(fork_pid, &status, 0);
      if(term_pid == -1) {
        perror("waitpid failed!");
        remove(filename);
        exit(-1);
      } else {
        if (WIFEXITED(status)) {
          exit_status = WEXITSTATUS(status);
        } else{
          exit_status = -1;
        }
      }
    }

    // set the file back to start for reading
    tmpfile = fopen(filename, "r");
    if(tmpfile == NULL) {
      perror("fopen failed!");
      remove(filename);
      exit(-1);
    }

    // read from the file and send it back to the client
    while ((c = fgetc(tmpfile)) != EOF) {
      rc = Socket_putc(c, connect_socket);
      if (rc == EOF) {
        // assume socket EOF ends service for this client
        printf("Socket_putc EOF or error\n");
        remove(filename);
        return;
      }
    }

    // close the tempfile
    fclose(tmpfile);

    sprintf(info_string, "Exit status: %d\n", exit_status);

    // +1 for including '\0'
    len = strlen(info_string) + 1;
    for(i = 0; i < len; i++) {
      rc = Socket_putc(info_string[i], connect_socket);
      if (rc == EOF) {
        // assume socket EOF ends service for this client
        printf("Socket_putc EOF or error\n");
        remove(filename);
        return;
      }
    }
  } 
  // end while loop of the service process

  // remove the temporary file
  remove(filename);
}
