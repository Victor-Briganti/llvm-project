#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char ** const argv) {
	int x = 0;

#pragma omp perfo
	for (int i = 0; i < atoi(argv[1]); i++) {
		x++;
	}

	printf("%d\n", x);
}