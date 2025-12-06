// RUN: %clang_cc1 -triple=x86_64-linux-gnu -verify -fopenmp -x c -std=c99 %s
// expected-no-diagnostics

void test_approx_valid(void) {
  int x = 0;
#pragma omp approx
  {
    x = 1;
  }
}

void test_approx_multiple_statements(void) {
  int a = 0, b = 0;
#pragma omp approx
  {
    a = 1;
    b = 2;
  }
}

void test_approx_nested_loop(void) {
  int sum = 0;
#pragma omp approx
  {
    for (int i = 0; i < 10; i++) {
      sum += i;
    }
  }
}
