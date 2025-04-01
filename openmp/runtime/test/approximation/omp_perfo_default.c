#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int x = 0;
#pragma omp parallel num_threads(4)
  {
#pragma omp approx for perfo(default, 3) reduction(+ : x)
    {
      for (int i = 0; i < atoi(argv[1]); i++)
        x++;
    }
  }
  printf("%d\n", x);
}
