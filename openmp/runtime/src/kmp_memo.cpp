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
#include "kmp_os.h"

#define MAP_SIZE 42
#define MAX_BUCKET_SIZE 4096

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

/* ------------------------------------------------------------------------ */

class kmp_map_t {
  enum bucket_state_t { BUCKET_EMPTY = 0, BUCKET_FULL = 1, BUCKET_DELETED = 2 };

  struct bucket_t {
    bucket_state_t state;
    kmp_int32 key;
    kmp_int32 size;
    kmp_int32 pos;
    kmp_int32 refs;
    kmp_int32 max_refs;
    bool is_full;
    memo_num_t dtype;
    void *data;
  };

  // Based on the hash algorithm:
  // <http://www.cse.yorku.ca/~oz/hash.html#djb2>
  kmp_int32 hash_func(kmp_int32 key) { return (5381 * 33 + key) % size; }

  void init(memo_num_t dtype) {
    entries = 0;
    size = MAP_SIZE;
    buckets = (bucket_t *)kmpc_calloc(size, sizeof(bucket_t));

    for (kmp_int32 i = 0; i < size; i++) {
      buckets[i].data = kmpc_calloc(MAX_BUCKET_SIZE, memo_sizeof(dtype));
      KMP_ASSERT2(buckets[i].data != NULL,
                  "Could not allocate memory for the data in the buckets");
    }

    KMP_ASSERT2(buckets != NULL,
                "Could not allocate memory for the map buckets");
  }

  void insert_data(bucket_t &bucket, void *data, memo_num_t dtype) {
    KMP_ASSERT2(data != NULL, "Inserted data should not be NULL");
    switch (dtype) {
    case memo_num_bool:
      ((bool *)bucket.data)[bucket.pos] = *(bool *)data;
      break;
    case memo_num_char:
    case memo_num_uchar:
      ((char *)bucket.data)[bucket.pos] = *(char *)data;
      break;
    case memo_num_wuchar:
    case memo_num_wchar:
      ((wchar_t *)bucket.data)[bucket.pos] = *(wchar_t *)data;
      break;
    case memo_num_char8:
      ((char *)bucket.data)[bucket.pos] = *(char *)data;
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

    if (bucket.pos == bucket.size - 1) {
      bucket.is_full = TRUE;
    }

    bucket.pos = (bucket.pos + 1) % bucket.size;
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

  bool insert(kmp_int32 hashloc, void *data, memo_num_t dtype,
              kmp_int32 max_refs) {
    kmp_int32 idx = hash_func(hashloc);
    kmp_int32 start_idx = idx;
    do {
      switch (buckets[idx].state) {
      case BUCKET_DELETED:
      case BUCKET_EMPTY:
        buckets[idx].key = hashloc;
        buckets[idx].state = BUCKET_FULL;
        buckets[idx].dtype = dtype;
        buckets[idx].max_refs = max_refs;
        buckets[idx].size = memo_sizeof(dtype);
        insert_data(buckets[idx], data, dtype);
        entries++;
        return true;
      case BUCKET_FULL:
        if (buckets[idx].key == hashloc) {
          insert_data(buckets[idx], data, dtype);
          return true;
        }
        idx = (idx + 1) % size;
      }
    } while (idx != start_idx);

    return false;
  }

  void *get(kmp_int32 hashloc, kmp_int32 &out_size) {
    kmp_int32 idx = hash_func(hashloc);
    kmp_int32 start_idx = idx;
    do {
      switch (buckets[idx].state) {
      case BUCKET_FULL:
        if (buckets[idx].key == hashloc) {
          if (!buckets[idx].is_full) {
            return FALSE;
          }

          buckets[idx].refs++;
          if (buckets[idx].refs >= buckets[idx].max_refs) {
            buckets[idx].is_full = FALSE;
            buckets[idx].refs = 0;
          }

          out_size = buckets[idx].size;
          return buckets[idx].data;
        }
        [[fallthrough]];
      case BUCKET_DELETED:
      case BUCKET_EMPTY:
        idx = (idx + 1) % size;
      }
    } while (idx != start_idx);

    return NULL;
  }

private:
  bucket_t *buckets;
  kmp_int32 entries;
  kmp_int32 size;

  static thread_local kmp_map_t *singleton;
};

thread_local kmp_map_t *kmp_map_t::singleton = nullptr;

/* ------------------------------------------------------------------------ */

int __kmp_memo_in(kmp_int32 hashloc, kmp_int32 gtid, kmp_int32 max_refs,
                  void *data, memo_num_t dtype) {
  kmp_map_t *map = kmp_map_t::create(dtype);
  kmp_int32 out_size = 0;
  void *out_data = map->get(hashloc, out_size);
  if (out_data == NULL) {
    return 1;
  }

  memo_interpolate(out_data, out_size, dtype, data);
  return 0;
}

void __kmp_memo_out(kmp_int32 hashloc, kmp_int32 gtid, kmp_int32 max_refs,
                    void *data, memo_num_t dtype) {
  kmp_map_t *map = kmp_map_t::create(dtype);
  map->insert(hashloc, data, dtype, max_refs);
}
