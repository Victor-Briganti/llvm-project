#include <stdio.h>
#include <stdlib.h>
// #include "omp_testsuite.h"

int test_omp_for_schedule_perfo() {
  int x = 0, y = 0, z = 0, i = 0;
  int iter = rand() % 20000;
  for (i = 0; i < iter; i++)
    x++;

#pragma omp parallel
  {
#pragma omp for schedule(perfo) reduction(+ : y)
    {
      for (i = 0; i < iter; i++)
        y++;
    }

#pragma omp for schedule(perfo, 100) reduction(+ : z)
    {
      for (i = 0; i < iter; i++)
        z++;
    }
  }

  if (y < (x / 2) || z < (x / 2))
    return 0;

  return 1;
}

int main() {
  int num_failed = 0;

  if (!test_omp_for_schedule_perfo())
    num_failed++;

  return num_failed;
}