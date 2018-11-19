#include <stdio.h>

#define UNUSED(x) ((void)(x))

int main(int argc, char **argv)
{
	UNUSED(argc);
	UNUSED(argv);
	puts("client");
	return 0;
}
