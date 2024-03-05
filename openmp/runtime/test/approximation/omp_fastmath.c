#include <stdlib.h>
#include <stdio.h>

#define ARRAY_SIZE 2048 * 2
double a[ARRAY_SIZE];

double foo(void) {
  double sum = 0.0f;
#pragma omp parallel shared(sum)
  {
#pragma omp for reduction(+ : sum)
    for (int i = 0; i < ARRAY_SIZE; ++i) {
#pragma omp approx fastmath
      { sum += a[i]; }
    }
  }
  return sum;
}

int main() {
  for (int i = 0; i < ARRAY_SIZE; i++)
    a[i] = rand() / 7.003;

  printf("%0.12f\n", foo());
}