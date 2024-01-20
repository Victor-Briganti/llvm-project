/*
 * kmp_approx.h - approximate runtime support
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef KMP_APPROX_H
#define KMP_APPROX_H

#include "kmp.h"
#include <stddef.h>

#define KMP_MAP_INIT_SIZE 40
#define KMP_MAP_LOAD 0.75

typedef enum {
  UNINITIALIZED = -1,
  INVALID = 0,
  VALID = 1,
} cache_state;

struct kmp_memo_cache {
  char *loc;
  void **addresses;
  void **datas;
  size_t *sizes;
  kmp_int32 nvars;
  kmp_real64 thresh;
  cache_state valid; /* 1 for valid. 0 for invalid */
  void construct(const char *loc, kmp_int32 nvars, kmp_real64 threshold);
  void insert(kmp_int32 idx, void *data, size_t size);
  void update_address();
  void update_cache();
  void destruct();
};

class kmp_memo_map {
  kmp_memo_cache **buckets;
  kmp_int32 nbuckets;
  kmp_int32 entries;

  kmp_int32 bucket_index(const char *loc);
  void rehash();

public:
  kmp_memo_map();
  ~kmp_memo_map();
  void insert(kmp_memo_cache *cache);
  kmp_memo_cache *search(const char *loc);
};

#endif // KMP_APPROX_H