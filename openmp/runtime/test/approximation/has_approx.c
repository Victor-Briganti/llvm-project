#include <stdlib.h>

int test_approx() {
  int x = 0;
#pragma omp approx
  {
    x = 1;
  }

  return x;
}

int main() {
  if (!test_approx()) {
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
