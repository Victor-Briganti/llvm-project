// RUN: %clang_cc1 -triple=x86_64-linux-gnu -verify -fopenmp -x c -std=c99 %s
// expected-no-diagnostics

void test_approx_memo(void) {
  int sum = 0;
#pragma omp approx memo(10) 
  {
    for (int i = 0; i < 10; i++) {
      sum++;
    }
  }
}
