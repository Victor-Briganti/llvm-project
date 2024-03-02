#include <stdio.h>
#include <stdlib.h>

int main() {
  int x = 0;
#pragma omp parallel num_threads(4)
{
#pragma omp approx for perfo(large, 100) reduction(+ : x)
  {
    for (int i = 0; i < 1024; i++) {
      x++;
      // printf("%d\n", i);
    }
  }
}
printf("%d\n", x);
}