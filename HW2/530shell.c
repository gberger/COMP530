/*
   GUILHERME DE CAMPOS LIMA BERGER
   PID 720535294
  
   I strive to uphold the University values of respect, 
   responsibility, discovery, and excellence. On my honor, 
   I pledge that I have neither given nor received 
   unauthorized assistance on this work.

   COMP530 - OPERATING SYSTEMS - FALL 2014
   Homework 2
   Purpose:
     Emulate a simple Linux shell.
     Commands can be entered as program names and parameters
     separated by whitespace.
     The program will receive the parameters and execute.
     After execution, there will be a prompt for the next command.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

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
		res = realloc(res, sizeof (char*) * ++n_spaces);

		if(res == NULL){
			fprintf(stderr, "Error while allocating!\n");
			exit(1);
		}

		res[n_spaces-1] = p;
		p = strtok(NULL, sep);
	}

	// Extend one more time, for the final NULL pointer
	res = realloc(res, sizeof (char*) * (n_spaces+1));

	if(res == NULL){
		fprintf(stderr, "Error while allocating!\n");
		exit(1);
	}

	res[n_spaces] = NULL;

	return res;
}

/*
	Given a string `s`, returns a pointer to its
	first non-whitespace location.
	Whitespace, as defined by `isspace`, is any of
	' ', '\t', '\n', '\v', '\f', '\r'
*/
char *ltrim(char *s) {
	while(isspace(*s)) s++;
	return s;
}

/* 
	Given a string `s`, modify the string so that
	its last character (before \0) is not a
	whitespace character, as defined by `isspace` (see `ltrim`).
*/
char *rtrim(char *s) {
	char* back = s + strlen(s);
	while(isspace(*--back));
	*(back+1) = '\0';
	return s;
}

/*
	Performs `ltrim` and `rtrim`.
*/
char *trim(char *s) {
	return rtrim(ltrim(s)); 
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

	possible = (char*)malloc(sizeof(char) * (strlen(cwd) + 1 + strlen(cmd) + 1));

	if(possible == NULL){
		fprintf(stderr, "Error while allocating!\n");
		exit(1);
	}

	sprintf(possible, "%s/%s", cwd, cmd);

	// `possible` starts out in the cwd.
	// If it is executable, it's in the currect working directory, return it.
	if(check_executable(possible)){
		return possible;
	}

	// Try each path.
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

void main_loop(void) {
	char *line = NULL;
	char *trimmed;
	size_t len = 0;	//dummy

	char **argv = NULL;
	pid_t pid;
	int status;	//dummy

	char **paths = get_paths();
	char *exec_path;

	printf("%% ");
	while (getline(&line, &len, stdin) != -1) {
		pid = fork();

		if(pid < 0) {
			//error!
			fprintf(stderr, "Error while forking!\n");
			exit(1);
		} else if(pid == 0) {
			trimmed = trim(line);
			argv = split_separator(trimmed, " \t\n\v\f\r");
			exec_path = search_path(argv[0], paths);

			if(exec_path != NULL) {
				execv(exec_path, argv);
				// if we are here, an error has occurred
				fprintf(stderr, "Error while executing!\n");
				exit(1);
			} else {
				printf("%s: command not found\n", argv[0]);
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
	free(paths);
}

int main(void) {
	main_loop();
	return 0;
}