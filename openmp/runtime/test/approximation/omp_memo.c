// RUN: %libomp-compile-and-run
#include <stdlib.h>
#include <stdio.h>

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
    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p)
      { a++, b++, c++, d++, e++, f++, g++, h++, m++, n++, o++, p++; }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z)
      { x++, y++, z++; }
    }
  }

  if (a == (saved[0] + 1) && b == (saved[1] + 1) && c == (saved[2] + 1) &&
      d == (saved[3] + 1) && e == (saved[4] + 1) && f == (saved[5] + 1) &&
      g == (saved[6] + 1) && h == (saved[7] + 1) && m == (saved[8] + 1) &&
      n == (saved[9] + 1) && o == (saved[10] + 1) && p == (saved[11] + 1) &&
      x == (saved[12] + 1) && y == (saved[13] + 1) && z == (saved[14] + 1))
    return 1;

  return 0;
}

int main() {
  int num_failed = 0;

  if (!test_omp_memo())
    num_failed++;

  return num_failed;
}
