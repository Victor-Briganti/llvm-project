// RUN: %clang_cc1 -fopenmp -x c++ -std=c++11 -triple x86_64-unknown-unknown -emit-llvm-bc %s -o %t.bc

void test_for_approx() {
#pragma omp for approx perfo(default, 1)
  for (int i = 0; i < 16; ++i) {
  }
}

void test_for_approx_schedule() {
#pragma omp for approx perfo(default, 1) schedule(static, 4)
  for (int i = 0; i < 16; ++i) {
  }
}

void test_parallel_for_approx() {
#pragma omp parallel for approx perfo(default, 1)
  for (int i = 0; i < 16; ++i) {
  }
}

void test_parallel_for_approx_schedule() {
#pragma omp parallel for approx perfo(default, 1) schedule(static, 4)
  for (int i = 0; i < 16; ++i) {
  }
}
