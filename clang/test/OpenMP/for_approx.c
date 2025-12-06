// RUN: %clang_cc1 -fopenmp -x c++ -std=c++11 -triple x86_64-unknown-unknown -emit-llvm-bc %s -o %t.bc

void test_for_approx() {
#pragma omp for approx
  for (int i = 0; i < 16; ++i) {
  }
}
