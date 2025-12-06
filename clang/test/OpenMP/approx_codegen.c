// RUN: %clang_cc1 -triple=x86_64-linux-gnu -fopenmp -emit-llvm -x c -std=c99 %s -o - | FileCheck %s

// CHECK-LABEL: @test_approx_simple
// CHECK-LABEL: @__captured_stmt

int test_approx_simple(void) {
  int x = 0;
#pragma omp approx
  {
    x = 1;
  }
  return x;
}

int main(void) {
  return test_approx_simple();
}
