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
#include <sys/types.h>
#include <sys/stat.h>

char **split(char *str, char *sep) {
	char **res = NULL;
	char *p;
	int n_spaces = 0;

	p = strtok(str, sep);
	while(p) {
		res = realloc(res, sizeof (char*) * ++n_spaces);
		res[n_spaces-1] = p;
		p = strtok(NULL, sep);
	}

	res = realloc(res, sizeof (char*) * (n_spaces+1));
	res[n_spaces] = NULL;

	return res;
}

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

int check_executable(char *path) {
	struct stat sb;
	return stat(path, &sb) == 0 && sb.st_mode & S_IXUSR;
}

char *search_path(char *cmd, char **paths){
	char *possible = (char*)malloc(sizeof(char) * (strlen(cmd) + 1));
	strcpy(possible, cmd);

	if(check_executable(possible)){
		return possible;
	}

	while(*paths != NULL){
		possible = (char*)realloc(possible, (sizeof(char)) * (strlen(*paths) + 1 + strlen(cmd) + 1));
		sprintf(possible, "%s/%s", *paths, cmd);

		if(check_executable(possible)) {
			return possible;
		}

		paths++;
	}

	return NULL;
}

void main_loop(void) {
	char *line = NULL;
	char *trimmed;
	size_t len = 0;

	char **cmd = NULL;
	pid_t pid;
	int status;

	char *path = getenv("PATH");
	char **paths = split(path, ":");

	char *exec_path;

	printf("%% ");
	while (getline(&line, &len, stdin) != -1) {
		pid = fork();

		if(pid == 0) {
			trimmed = trim(line);
			cmd = split(trimmed, " ");
			exec_path = search_path(cmd[0], paths);

			if(exec_path != NULL) {
				execv(exec_path, cmd);
			} else {
				printf("%s: command not found\n", cmd[0]);
				exit(1);
			}
		} else {
			waitpid(pid, &status, 0);
			printf("%% ");			
		}
	}

	if (line) {
		free(line);
	}
}

int main(void) {
	main_loop();
	return 0;
}