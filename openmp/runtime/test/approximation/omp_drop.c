#include <stdio.h>

int main() {
  int x = 0;
#pragma omp parallel
#pragma omp single
#pragma omp approx taskloop drop(5) reduction(+ : x)
  {
    for (int i = 0; i < 1000; i++)
      x++;
  }

  printf("%d\n", x);
  return 0;
}
