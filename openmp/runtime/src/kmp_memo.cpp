/*
 * kmp_memo.cpp -- memo approximatted structures.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "kmp_memo.h"
#include "kmp.h"
#include "kmp_debug.h"
#include <cmath>
#include <cstring>

#define MAP_SIZE 43
#define MAX_BUCKET_SIZE 512
#define MAX_WINDOW_SIZE (1 << 20)

#define MEMO_WARMUP_THRESHOLD 10
#define MEMO_OVERHEAD_CHECK_INTERVAL 100
#define MEMO_OVERHEAD_TOLERANCE 1.05

static size_t memo_sizeof(memo_num_t dtype) {
  switch (dtype) {
  case memo_num_bool:
    return sizeof(bool);
  case memo_num_char:
  case memo_num_uchar:
    return sizeof(char);
  case memo_num_wuchar:
  case memo_num_wchar:
    return sizeof(wchar_t);
  case memo_num_char8:
    return sizeof(char);
  case memo_num_char16:
    return sizeof(char16_t);
  case memo_num_char32:
    return sizeof(char32_t);
  case memo_num_ushort:
  case memo_num_short:
    return sizeof(short);
  case memo_num_uint:
  case memo_num_int:
    return sizeof(int);
  case memo_num_ulong:
  case memo_num_long:
    return sizeof(long);
  case memo_num_ulonglong:
  case memo_num_longlong:
    return sizeof(long long);
  case memo_num_float:
    return sizeof(float);
  case memo_num_double:
    return sizeof(double);
  case memo_num_longdouble:
    return sizeof(long double);
  default:
    KMP_ASSERT2(0, "Invalid data type for memoization");
  }
}

static void memo_interpolate(void *data, kmp_int32 size, memo_num_t dtype,
                             void *result) {
  KMP_ASSERT2(data != NULL, "Output data should not be NULL");
  KMP_ASSERT2(size != 0,
              "Size of the data should be greater than 0 for interpolation");

  double sum = 0.0;
  for (kmp_int32 i = 0; i < size; i++) {
    switch (dtype) {
    case memo_num_bool:
      sum += ((const bool *)data)[i] ? 1.0 : 0.0;
      break;
    case memo_num_char:
      sum += (double)((const char *)data)[i];
      break;
    case memo_num_uchar:
      sum += (double)((const unsigned char *)data)[i];
      break;
    case memo_num_short:
      sum += (double)((const short *)data)[i];
      break;
    case memo_num_ushort:
      sum += (double)((const unsigned short *)data)[i];
      break;
    case memo_num_int:
      sum += (double)((const int *)data)[i];
      break;
    case memo_num_uint:
      sum += (double)((const unsigned int *)data)[i];
      break;
    case memo_num_long:
      sum += (double)((const long *)data)[i];
      break;
    case memo_num_ulong:
      sum += (double)((const unsigned long *)data)[i];
      break;
    case memo_num_longlong:
      sum += (double)((const long long *)data)[i];
      break;
    case memo_num_ulonglong:
      sum += (double)((const unsigned long long *)data)[i];
      break;
    case memo_num_float:
      sum += (double)((const float *)data)[i];
      break;
    case memo_num_double:
      sum += ((const double *)data)[i];
      break;
    case memo_num_longdouble:
      sum += (double)((const long double *)data)[i];
      break;
    default:
      KMP_ASSERT2(0, "Invalid data type for memoization interpolation");
    }
  }

  double mean = sum / (double)size;

  switch (dtype) {
  case memo_num_bool:
    *((bool *)result) = mean >= 0.5;
    break;
  case memo_num_char:
    *((char *)result) = (char)mean;
    break;
  case memo_num_uchar:
    *((unsigned char *)result) = (unsigned char)mean;
    break;
  case memo_num_short:
    *((short *)result) = (short)mean;
    break;
  case memo_num_ushort:
    *((unsigned short *)result) = (unsigned short)mean;
    break;
  case memo_num_int:
    *((int *)result) = (int)mean;
    break;
  case memo_num_uint:
    *((unsigned int *)result) = (unsigned int)mean;
    break;
  case memo_num_long:
    *((long *)result) = (long)mean;
    break;
  case memo_num_ulong:
    *((unsigned long *)result) = (unsigned long)mean;
    break;
  case memo_num_longlong:
    *((long long *)result) = (long long)mean;
    break;
  case memo_num_ulonglong:
    *((unsigned long long *)result) = (unsigned long long)mean;
    break;
  case memo_num_float:
    *((float *)result) = (float)mean;
    break;
  case memo_num_double:
    *((double *)result) = mean;
    break;
  case memo_num_longdouble:
    *((long double *)result) = (long double)mean;
    break;
  default:
    KMP_ASSERT2(0, "Invalid data type for memoization interpolation");
  }
}

static double calculate_error(void *predicted, void *real, memo_num_t dtype) {
  double p = 0.0, r = 0.0;
  switch (dtype) {
  case memo_num_bool:
    p = *(bool *)predicted ? 1.0 : 0.0;
    r = *(bool *)real ? 1.0 : 0.0;
    break;
  case memo_num_char:
  case memo_num_char8:
    p = *(char *)predicted;
    r = *(char *)real;
    break;
  case memo_num_uchar:
    p = *(unsigned char *)predicted;
    r = *(unsigned char *)real;
    break;
  case memo_num_short:
    p = *(short *)predicted;
    r = *(short *)real;
    break;
  case memo_num_ushort:
    p = *(unsigned short *)predicted;
    r = *(unsigned short *)real;
    break;
  case memo_num_int:
    p = *(int *)predicted;
    r = *(int *)real;
    break;
  case memo_num_uint:
    p = *(unsigned int *)predicted;
    r = *(unsigned int *)real;
    break;
  case memo_num_long:
    p = *(long *)predicted;
    r = *(long *)real;
    break;
  case memo_num_ulong:
    p = *(unsigned long *)predicted;
    r = *(unsigned long *)real;
    break;
  case memo_num_longlong:
    p = *(long long *)predicted;
    r = *(long long *)real;
    break;
  case memo_num_ulonglong:
    p = *(unsigned long long *)predicted;
    r = *(unsigned long long *)real;
    break;
  case memo_num_float:
    p = *(float *)predicted;
    r = *(float *)real;
    break;
  case memo_num_double:
    p = *(double *)predicted;
    r = *(double *)real;
    break;
  case memo_num_longdouble:
    p = *(long double *)predicted;
    r = *(long double *)real;
    break;
  case memo_num_wuchar:
  case memo_num_wchar:
    p = *(wchar_t *)predicted;
    r = *(wchar_t *)real;
    break;
  case memo_num_char16:
    p = *(char16_t *)predicted;
    r = *(char16_t *)real;
    break;
  case memo_num_char32:
    p = *(char32_t *)predicted;
    r = *(char32_t *)real;
    break;
  default:
    KMP_ASSERT2(0, "Invalid data type for error calc");
  }

  if (r == 0.0)
    return (p == 0.0) ? 0.0 : 100.0;
  return (std::abs(p - r) / std::abs(r)) * 100.0;
}

/* ------------------------------------------------------------------------ */

class kmp_map_t {
public:
  enum bucket_state_t {
    BUCKET_EMPTY = 0,
    BUCKET_WARMUP = 1,
    BUCKET_COMPUTING = 2,
    BUCKET_PREDICTING = 3,
    BUCKET_VALIDATING = 4,
    BUCKET_DISABLED = 5
  };

  struct bucket_t {
    bucket_state_t state;
    kmp_int32 key;
    kmp_int32 size;
    kmp_int32 pos;
    kmp_int32 history_count;

    kmp_int32 window_size;
    kmp_int32 predict_count;
    double threshold;

    memo_num_t dtype;
    void *data;
    void *predicted_data;
    kmp_int32 failed_count;

    kmp_uint64 baseline_ticks;
    kmp_uint64 memo_ticks;
    kmp_uint64 start_ticks;
    kmp_int32 warmup_calls;
    kmp_int32 memo_calls;
  };

private:
  // Based on the hash algorithm:
  // <http://www.cse.yorku.ca/~oz/hash.html#djb2>
  kmp_int32 hash_func(kmp_int32 key) { return (5381 * 33 + key) % size; }

  void init(memo_num_t dtype) {
    entries = 0;
    size = MAP_SIZE;
    buckets = (bucket_t *)kmpc_calloc(size, sizeof(bucket_t));

    for (kmp_int32 i = 0; i < size; i++) {
      buckets[i].state = BUCKET_EMPTY;
      buckets[i].data = kmpc_calloc(MAX_BUCKET_SIZE, memo_sizeof(dtype));
      buckets[i].predicted_data = kmpc_calloc(1, memo_sizeof(dtype));
      buckets[i].failed_count = 0;
      buckets[i].baseline_ticks = 0;
      buckets[i].memo_ticks = 0;
      buckets[i].warmup_calls = 0;
      buckets[i].memo_calls = 0;
      KMP_ASSERT2(buckets[i].data != NULL,
                  "Could not allocate memory for the data in the buckets");
      KMP_ASSERT2(buckets[i].predicted_data != NULL,
                  "Could not allocate memory for the predicted data in the buckets");
    }

    KMP_ASSERT2(buckets != NULL,
                "Could not allocate memory for the map buckets");
  }

public:
  kmp_map_t() = delete;
  kmp_map_t(const kmp_map_t &) = delete;
  kmp_map_t &operator=(const kmp_map_t &) = delete;

  static kmp_map_t *create(memo_num_t dtype) {
    if (!singleton) {
      singleton = (kmp_map_t *)kmpc_malloc(sizeof(kmp_map_t));
      KMP_ASSERT2(singleton != NULL,
                  "Could not allocate memory for the map structure");
      singleton->init(dtype);
    }
    return singleton;
  }

  bucket_t *get_bucket(kmp_int32 hashloc, double thresh, memo_num_t dtype) {
    kmp_int32 idx = hash_func(hashloc);
    kmp_int32 start_idx = idx;
    do {
      if (buckets[idx].state == BUCKET_EMPTY) {
        buckets[idx].key = hashloc;
        buckets[idx].state = BUCKET_WARMUP;
        buckets[idx].dtype = dtype;
        buckets[idx].threshold = thresh;
        buckets[idx].size = memo_sizeof(dtype);
        buckets[idx].history_count = 0;
        buckets[idx].pos = 0;
        entries++;
        return &buckets[idx];
      }
      if (buckets[idx].key == hashloc) {
        return &buckets[idx];
      }
      idx = (idx + 1) % size;
    } while (idx != start_idx);

    return NULL; // Map full
  }

  void insert_data(bucket_t &bucket, void *data, memo_num_t dtype) {
    KMP_ASSERT2(data != NULL, "Inserted data should not be NULL");
    switch (dtype) {
    case memo_num_bool:
      ((bool *)bucket.data)[bucket.pos] = *(bool *)data;
      break;
    case memo_num_char:
    case memo_num_uchar:
    case memo_num_char8:
      ((char *)bucket.data)[bucket.pos] = *(char *)data;
      break;
    case memo_num_wuchar:
    case memo_num_wchar:
      ((wchar_t *)bucket.data)[bucket.pos] = *(wchar_t *)data;
      break;
    case memo_num_char16:
      ((char16_t *)bucket.data)[bucket.pos] = *(char16_t *)data;
      break;
    case memo_num_char32:
      ((char32_t *)bucket.data)[bucket.pos] = *(char32_t *)data;
      break;
    case memo_num_ushort:
    case memo_num_short:
      ((short *)bucket.data)[bucket.pos] = *(short *)data;
      break;
    case memo_num_uint:
    case memo_num_int:
      ((int *)bucket.data)[bucket.pos] = *(int *)data;
      break;
    case memo_num_ulong:
    case memo_num_long:
      ((long *)bucket.data)[bucket.pos] = *(long *)data;
      break;
    case memo_num_ulonglong:
    case memo_num_longlong:
      ((long long *)bucket.data)[bucket.pos] = *(long long *)data;
      break;
    case memo_num_float:
      ((float *)bucket.data)[bucket.pos] = *(float *)data;
      break;
    case memo_num_double:
      ((double *)bucket.data)[bucket.pos] = *(double *)data;
      break;
    case memo_num_longdouble:
      ((long double *)bucket.data)[bucket.pos] = *(long double *)data;
      break;
    default:
      KMP_ASSERT2(0, "Invalid data type for memoization");
    }

    bucket.pos = (bucket.pos + 1) % MAX_BUCKET_SIZE;
    if (bucket.history_count < MAX_BUCKET_SIZE) {
      bucket.history_count++;
    }
  }

private:
  bucket_t *buckets;
  kmp_int32 entries;
  kmp_int32 size;

  static thread_local kmp_map_t *singleton;
};

thread_local kmp_map_t *kmp_map_t::singleton = nullptr;

/* ------------------------------------------------------------------------ */

int __kmp_memo_in(kmp_int32 hashloc, kmp_int32 gtid, kmp_int32 threshold,
                  void *data, memo_num_t dtype) {
  kmp_map_t *map = kmp_map_t::create(dtype);
  kmp_map_t::bucket_t *bucket =
      map->get_bucket(hashloc, (double)threshold, dtype);

  if (bucket == NULL) {
    return 1;
  }

  bucket->start_ticks = KMP_NOW();

  if (bucket->state == kmp_map_t::BUCKET_DISABLED ||
      bucket->state == kmp_map_t::BUCKET_WARMUP) {
    return 1;
  }

  if (bucket->state == kmp_map_t::BUCKET_COMPUTING ||
      bucket->state == kmp_map_t::BUCKET_VALIDATING) {
    return 1;
  }

  if (bucket->state == kmp_map_t::BUCKET_PREDICTING) {
    if (bucket->predict_count < bucket->window_size) {
      bucket->predict_count++;
      std::memcpy(data, bucket->predicted_data, bucket->size);
      bucket->memo_ticks += (KMP_NOW() - bucket->start_ticks);
      bucket->memo_calls++;
      return 0;
    }

    bucket->state = kmp_map_t::BUCKET_VALIDATING;
    return 1;
  }
  return 1;
}

void __kmp_memo_out(kmp_int32 hashloc, kmp_int32 gtid,
                    kmp_int32 threshold, void *data, memo_num_t dtype) {
  kmp_map_t *map = kmp_map_t::create(dtype);
  kmp_map_t::bucket_t *bucket =
      map->get_bucket(hashloc, (double)threshold, dtype);

  if (bucket == NULL || bucket->state == kmp_map_t::BUCKET_DISABLED) {
    return;
  }

  kmp_uint64 elapsed = KMP_NOW() - bucket->start_ticks;

  if (bucket->state == kmp_map_t::BUCKET_WARMUP) {
    bucket->baseline_ticks += elapsed;
    bucket->warmup_calls++;
    if (bucket->warmup_calls >= MEMO_WARMUP_THRESHOLD) {
      bucket->state = kmp_map_t::BUCKET_COMPUTING;
    }
  } else {
    bucket->memo_ticks += elapsed;
    bucket->memo_calls++;

    if (bucket->memo_calls >= MEMO_OVERHEAD_CHECK_INTERVAL) {
      double avg_baseline = (double)bucket->baseline_ticks / bucket->warmup_calls;
      double avg_memo = (double)bucket->memo_ticks / bucket->memo_calls;
      if (avg_memo > avg_baseline * MEMO_OVERHEAD_TOLERANCE) {
        bucket->state = kmp_map_t::BUCKET_DISABLED;
        return;
      }
    }
  }

  if (bucket->state == kmp_map_t::BUCKET_COMPUTING) {
    map->insert_data(*bucket, data, dtype);
    bucket->state = kmp_map_t::BUCKET_PREDICTING;
    bucket->window_size = 10;
    bucket->predict_count = 0;
    memo_interpolate(bucket->data, bucket->history_count, dtype,
                     bucket->predicted_data);
  } else if (bucket->state == kmp_map_t::BUCKET_VALIDATING) {
    double error = calculate_error(bucket->predicted_data, data, dtype);

    if (error <= bucket->threshold) {
      if (bucket->window_size < MAX_WINDOW_SIZE) {
        bucket->window_size *= 2;
      }
      bucket->failed_count = 0;
    } else {
      bucket->window_size = 1;
      bucket->failed_count++;
      // Disable memoization if it fails too many times relative to threshold
      if (bucket->failed_count >
          (kmp_int32)(100.0 / (bucket->threshold + 1.0))) {
        bucket->state = kmp_map_t::BUCKET_DISABLED;
        return;
      }
    }

    map->insert_data(*bucket, data, dtype);
    bucket->state = kmp_map_t::BUCKET_PREDICTING;
    bucket->predict_count = 0;
    memo_interpolate(bucket->data, bucket->history_count, dtype,
                     bucket->predicted_data);
  }
}
