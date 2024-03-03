#include <stdio.h>

int main() {
  int x = 0;
#pragma omp parallel
#pragma omp approx taskloop
  {
    for (int i = 0; i < 1000; i++)
      x++;
  }

  printf("%d\n", x);
  return 0;
}
