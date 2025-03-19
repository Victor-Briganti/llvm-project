#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main() {
  int x = 0;
  
#pragma omp parallel reduction(+ : x)
{
  #pragma omp perfo
  for (int i = 0; i < 1024; i += 2)
    x++;
}

  printf("%d\n", x);
}
