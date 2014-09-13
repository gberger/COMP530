/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

char *ltrim(char *s) {
    while(isspace(*s)) s++;
    return s;
}

char *rtrim(char *s) {
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s) {
    return rtrim(ltrim(s)); 
}

void main_loop(void) {
	char *line = NULL;
	char *trimmed;
	size_t len = 0;

	char **cmd = NULL;
	char *p;
	int n_spaces = 0;
	pid_t pid;
	int status;

	printf("%% ");
	while (getline(&line, &len, stdin) != -1) {
		trimmed = trim(line);
		/* split line into argv parts */
		p = strtok(trimmed, " ");
		while (p) {
		  cmd = realloc(cmd, sizeof (char*) * ++n_spaces);
		  cmd[n_spaces-1] = p;
		  p = strtok(NULL, " ");
		}
		cmd = realloc(cmd, sizeof (char*) * (n_spaces+1));
		cmd[n_spaces] = 0;
		/* end split */

		pid = fork();
		if (pid == 0) {
			execvp(cmd[0], cmd+1);
		}
		waitpid(pid, &status, 0);

		cmd = NULL;
		n_spaces = 0;
		printf("%% ");
	}

	if (line) {
		free(line);
	}
}

int main(void) {
	main_loop();
	return 0;
}