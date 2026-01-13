// RUN: %clang_cc1 -triple=x86_64-linux-gnu -verify -fopenmp -x c -std=c99 %s
// expected-no-diagnostics

#include <math.h>

void test_approx_valid(void) {
  double x = 0.2;
  float y = 0.2;
#pragma omp approx fastmath
  {
    log2f(y);
    log2(x);
    logf(y);
    log(x);
    log10f(y);
    log10(x);
    exp2f(y);
    exp2(x);
    expf(y);
    exp(x);
    powf(y, y);
    pow(x, x);
    sinf(y);
    sin(x);
    cosf(y);
    cos(x);
    tanf(y);
    tan(x);
    asinf(y);
    asin(x);
    acosf(y);
    acos(x);
    atanf(y);
    atan(x);
    sinhf(y);
    sinh(x);
    coshf(y);
    cosh(x);
    tanhf(y);
    tanh(x);
    sqrtf(y);
    sqrt(x);
  }
}

void test_approx_multiple_statements(void) {
  int a = 0, b = 0;
#pragma omp approx  fastmath
  {
    a = 1;
    b = 2;
  }
}

void test_approx_loop(void) {
  int sum = 0;
#pragma omp for 
  {
    for (int i = 0; i < 10; i++) {
      #pragma omp approx fastmath
      sum += i;
    }
  }
}

void test_approx_parallel_loop(void) {
  int sum = 0;
#pragma omp parallel for
  {
    for (int i = 0; i < 10; i++) {
      #pragma omp approx fastmath
      sum += i;
    }
  }
}
