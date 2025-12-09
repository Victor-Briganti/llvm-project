// RUN: %clang_cc1 -fopenmp -x c++ -std=c++11 -triple x86_64-unknown-unknown -emit-llvm-bc %s -o %t.bc

void test_for_approx() {
#pragma omp for approx
  for (int i = 0; i < 16; ++i) {
  }
}

void test_for_approx_schedule() {
#pragma omp for approx schedule(static, 4)
  for (int i = 0; i < 16; ++i) {
  }
}


void test_parallel_for_approx() {
  int x = 0;

#pragma omp parallel for approx
  for (int i = 0; i < 16; ++i) {
    x++;
  }
}

void test_parallel_for_approx_schedule() {
  int x = 0;

#pragma omp parallel for approx schedule(static, 4)
  for (int i = 0; i < 16; ++i) {
    x++;
  }
}
