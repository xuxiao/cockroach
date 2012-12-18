#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define __USE_GNU
#include <dlfcn.h>

#include "targets.h"

int funcX(int a, int b)
{
	return a * b * (a - b) * (a + b);
}
int (*funcX_ptr)(int , int) = funcX;

int cmd_sum(int argc, char *argv[])
{
	int i;
	if (argc < 3) {
		fprintf(stderr,
		        "[%s] Number of arg.(%d) must be greater than 3.\n",
		        __func__, argc);
		return EXIT_FAILURE;
	}
	int num = atoi(argv[2]);
	int num_exec = 1;
	if (argc >= 4)
		num_exec = atoi(argv[3]);
	for (i = 0; i < num_exec; i++)
		printf("%d", sum_up_to(num));
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	if (argc < 2) {
		fprintf(stderr, "Error: missing a command.\n");
		return EXIT_FAILURE;
	}
	char *first_arg = argv[1];

	if (strcmp(first_arg, "hello") == 0) {
		printf("Hello, World!\n");
	} else if (strcmp(first_arg, "memcpy") == 0) {
		int dest[2] = {0, 0};
		int src[2] = {0xff, 0xaa};
		memcpy(dest, src, 2);
		printf("copied: %02x %02x\n", dest[0], dest[1]);
	} else if (strcmp(first_arg, "random") == 0) {
		printf("random: %ld\n", random());
	} else if (strcmp(first_arg, "setenv") == 0) {
		printf("setenv:  %d\n", setenv("foo", "1", 0));
	}
	else if (strcmp(first_arg, "hello2") == 0) {
		int count = 0;
		printf("Hello, World: %d\n", count++);
		printf("Hello, World: %d\n", count++);
	} else if (strcmp(first_arg, "funcX") == 0) {
		int a = (*funcX_ptr)(2, 3);
		printf("%d", a);
	} else if (strcmp(first_arg, "func1") == 0) {
		printf("%d", func1(1,2));
	}
	else if (strcmp(first_arg, "func1a") == 0) {
		printf("%d", func1a(1,2));
	}
	else if (strcmp(first_arg, "func1b") == 0) {
		printf("%d", func1b(1,2));
	}
	else if (strcmp(first_arg, "func2") == 0) {
		printf("%d", func2(1,2));
	}
	else if (strcmp(first_arg, "sum") == 0)
		ret = cmd_sum(argc, argv);
	else {
		fprintf(stderr, "Unknown command: %s\n", first_arg);
		ret = EXIT_FAILURE;
	}

	return ret;
}