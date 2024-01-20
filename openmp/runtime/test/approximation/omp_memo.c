// RUN: %libomp-compile-and-run
#include <stdlib.h>
#include <stdio.h>
#include "omp_testsuite.h"

int test_omp_memo() {
  int a, b, c, d, e, f, g, h, m, n, o, p, x, y, z;
  int saved[15];

  saved[0] = (a = rand() % 32);
  saved[1] = (b = rand() % 32);
  saved[2] = (c = rand() % 32);
  saved[3] = (d = rand() % 32);
  saved[4] = (e = rand() % 32);
  saved[5] = (f = rand() % 32);
  saved[6] = (g = rand() % 32);
  saved[7] = (h = rand() % 32);
  saved[8] = (m = rand() % 32);
  saved[9] = (n = rand() % 32);
  saved[10] = (o = rand() % 32);
  saved[11] = (p = rand() % 32);
  saved[12] = (x = rand() % 32);
  saved[13] = (y = rand() % 32);
  saved[14] = (z = rand() % 32);

#pragma omp parallel
  {
    for (int i = 0; i < LOOPCOUNT; i++) {
#pragma omp memo shared(a, b, c, d, e, f, g, h, m, n, o, p)
      { a++, b++, c++, d++, e++, f++, g++, h++, m++, n++, o++, p++; }
    }

    for (int i = 0; i < LOOPCOUNT; i++) {
#pragma omp memo shared(x, y, z)
      { x++, y++, z++; }
    }
  }

  if (a != saved[0] + 2 && b != saved[1] + 2 && c != saved[2] + 2 &&
      d != saved[3] + 2 && e != saved[4] + 2 && f != saved[5] + 2 &&
      g != saved[6] + 2 && h != saved[7] + 2 && m != saved[8] + 2 &&
      n != saved[9] + 2 && o != saved[10] + 2 && p != saved[11] + 2 &&
      x != saved[12] + 2 && y != saved[13] + 2 && z != saved[14] + 2)
    return 0;

  return 1;
}

int main() {
  int num_failed = 0;

  if (!test_omp_memo())
    num_failed++;

  return num_failed;
}