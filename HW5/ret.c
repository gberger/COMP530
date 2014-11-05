#include <stdlib.h>

int main(int argc, char const *argv[])
{
	if(argc < 2) {
		return 99;
	}
	return atoi(argv[1]);
}