#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main() {
  int x = 0;
  int y = 0;

#pragma unroll
  for (int i = 0; i < 100; i++) {
    x++;
    y++;
  }

  printf("%d\n", x);
  printf("%d\n", y);
}
