// RUN: %clang_cc1 -fopenmp -x c++ -std=c++11 -triple x86_64-unknown-unknown -emit-llvm-bc %s -o %t.bc

void test_for_approx() {
#pragma omp for approx perfo(large, 1)
  for (int i = 0; i < 16; ++i) {
  }
}

void test_for_approx_schedule() {
#pragma omp for approx perfo(large, 1) schedule(static, 4)
  for (int i = 0; i < 16; ++i) {
  }
}

void test_parallel_for_approx() {
#pragma omp parallel for approx perfo(large, 1)
  for (int i = 0; i < 16; ++i) {
  }
}

void test_parallel_for_approx_schedule() {
  #pragma omp parallel for approx perfo(large, 1) schedule(static, 4)
  for (int i = 0; i < 16; ++i) {
  }
}

void test_parallel_for_nested_approx() {
  #pragma omp parallel
  for (int i = 0; i < 16; ++i) {
    #pragma omp for approx perfo(large, 1)
    for (int j = 0; j < 17; j++) {
      for (int k = 0; k < 18; k++) {
      }
    }
  }
}

void test_for_parallel_nested_approx(int N) {
  #pragma omp for approx perfo(large, 1)
  for (int i = 0; i < 16; ++i) {
    #pragma omp parallel
    for (int j = 0; j < 17; j++) {
    }
  }
}
