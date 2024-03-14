#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main() {
  int x = 0;
#pragma omp parallel num_threads(4)
{
#pragma omp approx for perfo(large, 3) reduction(+ : x)
  {
    for (int i = 0; i < 1024; i++)
      x++;
  }
}
printf("%d\n", x);
}
