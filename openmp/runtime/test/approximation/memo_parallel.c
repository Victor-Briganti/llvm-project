/*
 * memo_parallel_test.c -- Test memoization with parallel regions.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// RUN: %libomp-compile && %libomp-run

#include <stdio.h>
#include <omp.h>

#define NUM_LOOPS 100000

int test_memo_region() {
  int x = 0;
  int y = 0;

  double start, end;

  start = omp_get_wtime();
  #pragma omp parallel for reduction(+ : x)
  for (int i = 0; i < NUM_LOOPS; i++) {
    #pragma omp approx memo(5) input(i) output(x)
    {
      x++;
    }
  }
  end = omp_get_wtime();
  printf("Memoized region time: %f\n", end - start);

  start = omp_get_wtime();
  #pragma omp parallel for reduction(+ : y)
  for (int i = 0; i < NUM_LOOPS; i++) {
    y++;
  }
  end = omp_get_wtime();
  printf("Standard region time: %f\n", end - start);

  printf("x = %d, y = %d\n", x, y);
  return 0;
}

int main() {
  test_memo_region();
  return 0;
}
