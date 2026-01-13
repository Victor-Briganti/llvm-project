//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "kmp.h"

#include <math.h>

#define M_LOG10_2 0.3010299956639812
#define M_ONE_LN2 1.4426950408889634

template <typename T> static T kernel_log2(T x) {
  constexpr int COEFF_SIZE = 10;
  static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                "Invalid type for this function");
  constexpr T coeffs[COEFF_SIZE] = {T(3.89729184e-05),  T(1.44211130e+00),
                                    T(-7.17371181e-01), T(4.64553842e-01),
                                    T(-3.15227816e-01), T(1.97374547e-01),
                                    T(-1.02013711e-01), T(3.89226868e-02),
                                    T(-9.47682649e-03), T(1.08818382e-03)};

  int exp;
  T acc, xp;

  // transform mantissa from [0.5, 1) -> [0, 1)
  T mantissa;
  if constexpr (std::is_same_v<T, float>) {
    mantissa = frexpf(x, &exp) * 2 - 1;
  } else if constexpr (std::is_same_v<T, double>) {
    mantissa = frexp(x, &exp) * 2 - 1;
  }
  exp = exp - 1;

  acc = coeffs[0] + coeffs[1] * mantissa;
  xp = mantissa * mantissa;

  for (int i = 2; i < COEFF_SIZE; i++) {
    acc += coeffs[i] * xp;
    xp *= mantissa;
  }

  return acc + static_cast<T>(exp);
}

template <typename T> static T kernel_asin(T x) {
  static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                "Invalid type for this function");
  constexpr T pi2 = static_cast<T>(M_PI_2);
  constexpr T one = static_cast<T>(1.0);

  if (x >= one)
    return pi2;

  if (x <= -one)
    return -pi2;

  T frac;
  if constexpr (std::is_same_v<T, float>) {
    frac = sqrtf(one - x * x);
  } else if constexpr (std::is_same_v<T, double>) {
    frac = sqrt(one - x * x);
  }

  if (frac < static_cast<T>(1e-7))
    return (x < 0) ? -pi2 : pi2;

  if constexpr (std::is_same_v<T, float>) {
    return __kmpc_omp_atanf(x / frac);
  } else if constexpr (std::is_same_v<T, double>) {
    return __kmpc_omp_atan(x / frac);
  }
}

template <typename T> static T kernel_atan(T x) {
  constexpr T pi2 = static_cast<T>(M_PI_2);
  constexpr T one = static_cast<T>(1.0);
  constexpr T c1 = static_cast<T>(0.33288950512027);
  constexpr T c2 = static_cast<T>(-0.08467922817644);
  constexpr T c3 = static_cast<T>(0.03252232640125);
  constexpr T c4 = static_cast<T>(-0.00749305860992);

  T offset = 0.0;
  x = -one / x;

  if (x > one) {
    offset = pi2;
  } else if (x < -one) {
    offset = -pi2;
  }

  T x2 = x * x;
  T poly = 1.0;
  poly += c1 * x2;
  T x4 = x2 * x2;
  poly += c2 * x4;
  poly += c3 * (x4 * x2);
  poly += c4 * (x4 * x4);

  return offset + (x / poly);
}

template <typename T> static T kernel_cos_pi(T x) {
  constexpr int COEFF_SIZE = 14;
  static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                "Invalid type for this function");
  constexpr T coeffs[COEFF_SIZE] = {
      T(9.99999992e-01),  T(-5.58095804e-16), T(-4.93480139e+00),
      T(1.36699826e-14),  T(4.05869825e+00),  T(-1.26448668e-13),
      T(-1.33517440e+00), T(4.71045125e-13),  T(2.35063254e-01),
      T(-8.27975054e-13), T(-2.53909919e-02), T(6.85880321e-13),
      T(1.60531764e-03),  T(-2.16178305e-13)};

  T acc, xp;
  T integer;
  if constexpr (std::is_same_v<T, float>) {
    integer = roundf(x / 2);
  } else if constexpr (std::is_same_v<T, double>) {
    integer = round(x / 2);
  }

  x -= 2 * integer;

  acc = coeffs[0] + coeffs[1] * x;
  xp = x * x;

  for (int i = 2; i < COEFF_SIZE; i++) {
    acc += coeffs[i] * xp;
    xp *= x;
  }

  return acc;
}

template <typename T> static T kernel_exp2(T x) {
  constexpr int COEFF_SIZE = 10;
  static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                "Invalid type for this function");
  constexpr T coeffs[COEFF_SIZE] = {T(1.00000000e+00), T(6.93147181e-01),
                                    T(2.40226507e-01), T(5.55041104e-02),
                                    T(9.61811830e-03), T(1.33339455e-03),
                                    T(1.53949984e-04), T(1.53693670e-05),
                                    T(1.22575650e-06), T(1.44242433e-07)};

  T acc, xp;
  T integer;
  if constexpr (std::is_same_v<T, float>) {
    integer = floorf(x);
  } else if constexpr (std::is_same_v<T, double>) {
    integer = floor(x);
  }
  const T decimal = x - integer;

  acc = coeffs[0] + coeffs[1] * decimal;
  xp = decimal * decimal;

  for (int i = 2; i < COEFF_SIZE; i++) {
    acc += coeffs[i] * xp;
    xp *= decimal;
  }

  if constexpr (std::is_same_v<T, float>) {
    return ldexpf(acc, static_cast<int>(integer));
  } else if constexpr (std::is_same_v<T, double>) {
    return ldexp(acc, static_cast<int>(integer));
  }
}

template <typename T> static T kernel_sin_pi(T x) {
  constexpr int COEFF_SIZE = 13;
  static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                "Invalid type for this function");
  constexpr T coeffs[COEFF_SIZE] = {
      T(1.55431223e-15),  T(3.14159173e+00),  T(-2.77555756e-16),
      T(-5.16768502e+00), T(-4.13280521e-14), T(2.54992664e+00),
      T(2.19456397e-13),  T(-5.98397406e-01), T(-4.23771695e-13),
      T(8.06047826e-02),  T(3.75954085e-13),  T(-6.04102785e-03),
      T(-1.23925516e-13)};

  T acc, xp;
  T integer;
  if constexpr (std::is_same_v<T, float>) {
    integer = roundf(x / 2);
  } else if constexpr (std::is_same_v<T, double>) {
    integer = round(x / 2);
  }
  x -= 2 * integer;

  acc = coeffs[0] + coeffs[1] * x;
  xp = x * x;

  for (int i = 2; i < COEFF_SIZE; i++) {
    acc += coeffs[i] * xp;
    xp *= x;
  }

  return acc;
}

template <typename T> static T kernel_sqrt(T x) {
  constexpr int COEFF_SIZE = 10;
  static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                "Invalid type for this function");
  constexpr T coeffs[COEFF_SIZE] = {T(1.00000000e+00),  T(4.99999815e-01),
                                    T(-1.24994769e-01), T(6.24357941e-02),
                                    T(-3.86363763e-02), T(2.56286488e-02),
                                    T(-1.60201623e-02), T(8.08000306e-03),
                                    T(-2.71359669e-03), T(4.34204965e-04)};
  int exp;
  T acc, xp;

  // transform mantissa from [0.5, 1) -> [0, 1)
  T mantissa;
  if constexpr (std::is_same_v<T, float>) {
    mantissa = frexpf(x, &exp) * 2 - 1;
  } else if constexpr (std::is_same_v<T, double>) {
    mantissa = frexp(x, &exp) * 2 - 1;
  }
  exp = exp - 1;

  acc = coeffs[0] + coeffs[1] * mantissa;
  xp = mantissa * mantissa;

  for (int i = 2; i < COEFF_SIZE; i++) {
    acc += coeffs[i] * xp;
    xp *= mantissa;
  }

  // An odd input exponent means an extra sqrt(2) in the output
  if (exp & 1)
    acc *= static_cast<T>(M_SQRT2);

  if constexpr (std::is_same_v<T, float>) {
    return acc * ldexpf(1, exp >> 1);
  } else if constexpr (std::is_same_v<T, double>) {
    return acc * ldexp(1, exp >> 1);
  }
}

/* ------------------------------------------------------------------------ */

float __kmpc_omp_log2f(float x) { return kernel_log2<float>(x); }

double __kmpc_omp_log2(double x) { return kernel_log2<double>(x); }

float __kmpc_omp_logf(float x) {
  return static_cast<float>(M_LN2) * kernel_log2<float>(x);
}

double __kmpc_omp_log(double x) {
  return static_cast<double>(M_LN2) * kernel_log2<double>(x);
}

float __kmpc_omp_log10f(float x) {
  return static_cast<float>(M_LOG10_2) * kernel_log2<float>(x);
}

double __kmpc_omp_log10(double x) {
  return static_cast<double>(M_LOG10_2) * kernel_log2<double>(x);
}

float __kmpc_omp_exp2f(float x) { return kernel_exp2<float>(x); }

double __kmpc_omp_exp2(double x) { return kernel_exp2<double>(x); }

float __kmpc_omp_expf(float x) {
  return kernel_exp2<float>(static_cast<float>(M_ONE_LN2) * x);
}

double __kmpc_omp_exp(double x) { return kernel_exp2<double>(M_ONE_LN2 * x); }

float __kmpc_omp_powf(float x, float y) {
  const float sign = (x < 0) ? -1.0f : 1.0f;
  return sign * __kmpc_omp_exp2f(y * __kmpc_omp_log2f(sign * x));
}

double __kmpc_omp_pow(double x, double y) {
  const double sign = (x < 0) ? -1.0 : 1.0;
  return sign * __kmpc_omp_exp2(y * __kmpc_omp_log2(sign * x));
}

float __kmpc_omp_sinf(float x) {
  return kernel_sin_pi(static_cast<float>(M_1_PI) * x);
}

double __kmpc_omp_sin(double x) { return kernel_sin_pi(M_1_PI * x); }

float __kmpc_omp_cosf(float x) {
  return kernel_cos_pi(static_cast<float>(M_1_PI) * x);
}

double __kmpc_omp_cos(double x) { return kernel_cos_pi<double>(M_1_PI * x); }

float __kmpc_omp_tanf(float x) { return __kmpc_omp_sinf(x) / __kmpc_omp_cosf(x); }

double __kmpc_omp_tan(double x) { return __kmpc_omp_sin(x) / __kmpc_omp_cos(x); }

float __kmpc_omp_asinf(float x) { return kernel_asin<float>(x); }

double __kmpc_omp_asin(double x) { return kernel_asin<double>(x); }

float __kmpc_omp_acosf(float x) {
  return static_cast<float>(M_PI_2) - __kmpc_omp_asinf(x);
}

double __kmpc_omp_acos(double x) { return M_PI_2 - __kmpc_omp_asin(x); }

float __kmpc_omp_atanf(float x) { return kernel_atan<float>(x); }

double __kmpc_omp_atan(double x) { return kernel_atan<double>(x); }

float __kmpc_omp_sinhf(float x) {
  const float ex = __kmpc_omp_expf(x);
  const float exn = __kmpc_omp_expf(-x);
  return (ex - exn) / 2;
}

double __kmpc_omp_sinh(double x) {
  const double ex = __kmpc_omp_exp(x);
  const double exn = __kmpc_omp_exp(-x);
  return (ex - exn) / 2;
}

float __kmpc_omp_coshf(float x) {
  const float ex = __kmpc_omp_expf(x);
  const float exn = __kmpc_omp_expf(-x);
  return (ex + exn) / 2;
}

double __kmpc_omp_cosh(double x) {
  const double ex = __kmpc_omp_expf(x);
  const double exn = __kmpc_omp_expf(-x);
  return (ex + exn) / 2;
}

float __kmpc_omp_tanhf(float x) {
  if (x < -19)
    return -1;

  if (x > 19)
    return 1;

  const float ex2 = __kmpc_omp_expf(x * 2);
  return (ex2 - 1) / (ex2 + 1);
}

double __kmpc_omp_tanh(double x) {
  if (x < -19)
    return -1;

  if (x > 19)
    return 1;

  const double ex2 = __kmpc_omp_exp(x * 2);
  return (ex2 - 1) / (ex2 + 1);
}

float __kmpc_omp_sqrtf(float x) { return kernel_sqrt<float>(x); }

double __kmpc_omp_sqrt(double x) { return kernel_sqrt<double>(x); }