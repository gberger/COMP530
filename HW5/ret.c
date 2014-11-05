#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
	printf("This is some output.\n");
	if(argc < 2) {
		return 99;
	}
	return atoi(argv[1]);
}