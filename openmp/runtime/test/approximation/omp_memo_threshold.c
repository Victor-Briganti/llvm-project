#include <stdlib.h>
#include <stdio.h>

int test_omp_memo0() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}

int test_omp_memo1() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}
int test_omp_memo2() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}
int test_omp_memo3() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}
int test_omp_memo4() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}
int test_omp_memo5() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}

int test_omp_memo6() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}

int test_omp_memo7() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}

int test_omp_memo8() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}

int test_omp_memo9() {
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
#pragma omp approx memo shared(a, b, c, d, e, f, g, h, m, n, o, p) threshold(20)
      {
        a += 100, b += 100, c += 100, d += 100, e += 100, f += 100, g += 100,
            h += 100, m += 100, n += 100, o += 100, p += 100;
      }
    }

    for (int i = 0; i < 1000; i++) {
#pragma omp approx memo shared(x, y, z) threshold(20)
      {
        x += 100, y += 100, z += 100;
      }
    }
  }

  if (a == (saved[0] + 500) && b == (saved[1] + 500) && c == (saved[2] + 500) &&
      d == (saved[3] + 500) && e == (saved[4] + 500) && f == (saved[5] + 500) &&
      g == (saved[6] + 500) && h == (saved[7] + 500) && m == (saved[8] + 500) &&
      n == (saved[9] + 500) && o == (saved[10] + 500) &&
      p == (saved[11] + 500) && x == (saved[12] + 500) &&
      y == (saved[13] + 500) && z == (saved[14] + 500))
    return 1;

  return 0;
}

int main() {
  int num_failed = 0;

  if (!test_omp_memo0())
    num_failed++;

  if (!test_omp_memo1())
    num_failed++;

  if (!test_omp_memo2())
    num_failed++;

  if (!test_omp_memo3())
    num_failed++;

  if (!test_omp_memo4())
    num_failed++;

  if (!test_omp_memo5())
    num_failed++;

  if (!test_omp_memo6())
    num_failed++;

  if (!test_omp_memo7())
    num_failed++;

  if (!test_omp_memo8())
    num_failed++;

  if (!test_omp_memo9())
    num_failed++;

  return num_failed;
}
