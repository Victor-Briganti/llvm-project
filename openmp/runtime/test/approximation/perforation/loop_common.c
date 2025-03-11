#include <stdio.h>
#include <stdlib.h>

int main() {
	int x = 0;

#pragma omp perfo
	for (int i = 0; i < 100; i++) {
		x++;
	}

	printf("%d\n", x);
}