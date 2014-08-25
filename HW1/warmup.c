#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINENUM 80

void clear_str(char* str, int size) {
	int i;
	for(i=0; i<size; i++) {
		str[i] = 0;
	}
}

void append_char(char* str, char c) {
	str[strlen(str)] = c;
}

void replace_last_char(char* str, char c) {
	str[strlen(str)-1] = c;
}

int main(void) {
	char str[LINENUM+1];
	char c;
	int last_was_star = 0;

	clear_str(str, LINENUM+1);

	while(1) {
		c = getchar();

		if(c == EOF) {
			exit(0);
		}

		if(c == '\n') {
			append_char(str, ' ');
		} else if(c == '*') {
			if(last_was_star) {
				replace_last_char(str, '^');
				last_was_star = 0;
			} else {
				append_char(str, c);
				last_was_star = 1;
			}
		} else {
			append_char(str, c);
		}

		
		if(strlen(str) == LINENUM){
			printf("%s", str);
			clear_str(str, LINENUM);
		}
	}

	return 0;
}
