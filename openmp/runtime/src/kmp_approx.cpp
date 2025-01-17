/*
 * kmp_approx.cpp -- KPTS runtime approximatted support library
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "kmp.h"
#include "kmp_lock.h"

#include <math.h>

#define KMP_MAP_INIT_SIZE 40
#define KMP_MAP_LOAD 0.75

namespace {

kmp_real64 convert(void *value, memo_num_t type) {
  switch (type) {
  case (memo_num_bool):
    return (kmp_real64)(*(bool *)value);
  case (memo_num_char):
    return (kmp_real64)(*(char *)value);
  case (memo_num_wchar):
  case (memo_num_wuchar):
    return (kmp_real64)(*(wchar_t *)value);
  case (memo_num_uchar):
  case (memo_num_char8):
    return (kmp_real64)(*(unsigned char *)value);
  case (memo_num_char16):
    return (kmp_real64)(*(char16_t *)value);
  case (memo_num_char32):
    return (kmp_real64)(*(char32_t *)value);
  case (memo_num_ushort):
    return (kmp_real64)(*(unsigned short *)value);
  case (memo_num_uint):
    return (kmp_real64)(*(unsigned int *)value);
  case (memo_num_int):
    return (kmp_real64)(*(int *)value);
  case (memo_num_short):
    return (kmp_real64)(*(short *)value);
  case (memo_num_ulong):
    return (kmp_real64)(*(unsigned long *)value);
  case (memo_num_long):
    return (kmp_real64)(*(long *)value);
  case (memo_num_ulonglong):
    return (kmp_real64)(*(unsigned long long *)value);
  case (memo_num_longlong):
    return (kmp_real64)(*(long long *)value);
  case (memo_num_float):
    return (kmp_real64)(*(float *)value);
  case (memo_num_double):
    return (kmp_real64)(*(double *)value);
  case (memo_num_longdouble):
    return (kmp_real64)(*(long double *)value);
  }

  KMP_ASSERT(0 && "Invalid number type");
  return 0;
}

/*----------------------------------------------------------------------------*/

enum cache_state {
  VALID = 1,
  INVALID = 0,
  UNINITIALIZED = -1,
};

struct kmp_memo_cache {
  void **addresses;
  void **datas;

  size_t *sizes;
  memo_num_t *types;

  kmp_int32 loc;
  kmp_int32 nvars;
  kmp_real64 thresh;
  cache_state state; /* 1 for valid. 0 for invalid */

  void construct(kmp_int32 loc, kmp_int32 nvars, kmp_real64 threshold) {
    this->loc = loc;
    this->nvars = nvars;

    this->sizes =
        static_cast<size_t *>(kmpc_malloc(sizeof(size_t) * this->nvars));
    KMP_ASSERT2(this->sizes != NULL, "fail to allocate");
    this->types = static_cast<memo_num_t *>(
        kmpc_malloc(sizeof(memo_num_t) * this->nvars));
    KMP_ASSERT2(this->types != NULL, "fail to allocate");
    this->datas =
        static_cast<void **>(kmpc_malloc(sizeof(void *) * this->nvars));
    KMP_ASSERT2(this->datas != NULL, "fail to allocate");
    this->addresses =
        static_cast<void **>(kmpc_malloc(sizeof(void *) * this->nvars));
    KMP_ASSERT2(this->addresses != NULL, "fail to allocate");

    for (kmp_int32 i = 0; i < nvars; i++) {
      this->sizes[i] = 0;
      this->types[i] = memo_num_undefined;
      this->datas[i] = NULL;
      this->addresses[i] = NULL;
    }

    this->thresh = threshold <= 0 ? 0 : threshold;
    this->state = UNINITIALIZED;
  }

  void destruct() {
    for (kmp_int32 i = 0; i < nvars; ++i) {
      kmpc_free(datas[i]);
    }

    kmpc_free(sizes);
    kmpc_free(types);
    kmpc_free(datas);
    kmpc_free(addresses);
    sizes = NULL;
    types = NULL;
    datas = NULL;
    addresses = NULL;
  }

  void insert(kmp_int32 idx, void *var, size_t size, memo_num_t type) {
    types[idx] = type;
    sizes[idx] = size;
    datas[idx] = kmpc_malloc(size);
    addresses[idx] = var;
  }

  void update_address() {
    for (kmp_int32 i = 0; i < this->nvars; i++)
      memcpy(addresses[i], datas[i], sizes[i]);
  }

  void update_cache() {
    for (kmp_int32 i = 0; i < this->nvars; i++)
      memcpy(datas[i], addresses[i], sizes[i]);
  }

  // Percentage difference formula:
  // fabs(x - y) / y
  void verify_validity() {
    kmp_real64 final = 0.0f;
    kmp_real64 *res =
        static_cast<kmp_real64 *>(kmpc_malloc(sizeof(kmp_real64) * nvars));
    KMP_ASSERT2(res == NULL, "fail to allocate");

    for (kmp_int32 i = 0; i < nvars; i++) {
      kmp_real64 data = convert(datas[i], types[i]);
      kmp_real64 address = convert(addresses[i], types[i]);
      res[i] = fabs(address - data) / data;
    }

    for (kmp_int32 i = 0; i < nvars; i++)
      final += res[i];

    final = final / (kmp_real64)nvars;
    state = (cache_state)islessequal(final, thresh);
  }
};

/*----------------------------------------------------------------------------*/

class kmp_memo_map {
  kmp_memo_cache **buckets;
  kmp_int32 nbuckets;
  kmp_int32 entries;

  kmp_int32 bucket_index(kmp_int32 loc) {
    kmp_int32 hash = 0;
    hash = loc + (loc << 6) + (hash << 16) - loc;
    return (hash % nbuckets);
  }

  void rehash() {
    kmp_memo_cache **old_buckets = buckets;
    kmp_int32 old_nbuckets = nbuckets;

    nbuckets *= 2;
    entries = 0;

    size_t bucket_size = sizeof(kmp_memo_cache *) * nbuckets;
    buckets = static_cast<kmp_memo_cache **>(kmpc_malloc(bucket_size));
    KMP_ASSERT2(buckets == NULL, "fail to allocate");

    for (kmp_int32 i = 0; i < nbuckets; i++)
      buckets[i] = NULL;

    for (kmp_int32 i = 0; i < old_nbuckets; i++) {
      if (old_buckets[i] != NULL) {
        insert(old_buckets[i]);
      }
    }

    kmpc_free(old_buckets);
  }

public:
  kmp_memo_map() {
    nbuckets = KMP_MAP_INIT_SIZE;
    entries = 0;

    size_t bucket_size = sizeof(kmp_memo_cache *) * KMP_MAP_INIT_SIZE;
    buckets = static_cast<kmp_memo_cache **>(kmpc_malloc(bucket_size));
    KMP_ASSERT2(buckets != NULL, "fail to allocate");

    for (kmp_int32 i = 0; i < KMP_MAP_INIT_SIZE; i++) {
      buckets[i] = NULL;
    }
  }

  ~kmp_memo_map() {
    for (kmp_int32 i = 0; i < nbuckets; i++) {
      if (buckets[i] != NULL) {
        buckets[i]->destruct();
        kmpc_free(buckets[i]);
        buckets[i] = NULL;
      }
    }

    kmpc_free(buckets);
    buckets = NULL;
  }

  void insert(kmp_memo_cache *cache) {
    kmp_int32 idx = bucket_index(cache->loc);
    kmp_int32 aidx = idx;

    while (buckets[aidx] != NULL) {
      aidx = (aidx + 1) % nbuckets;
      if (aidx == idx) {
        rehash();
        idx = bucket_index(cache->loc);
        aidx = idx;
      }
    }

    buckets[aidx] = cache;
    entries++;

    float fentries = static_cast<float>(entries);
    float fnbuckets = static_cast<float>(nbuckets);

    if ((fentries / fnbuckets) >= KMP_MAP_LOAD)
      rehash();
  }

  kmp_memo_cache *search(kmp_int32 loc) {
    kmp_int32 idx = bucket_index(loc);

    kmp_memo_cache *cache = buckets[idx];
    while (cache != NULL) {
      if (cache->loc == loc)
        return cache;

      idx = (idx + 1) % nbuckets;
      cache = buckets[idx];
    }

    return NULL;
  }
};

kmp_memo_map map;

/*----------------------------------------------------------------------------*/

class kmp_map_lock {
  kmp_lock_t lock;

public:
  kmp_map_lock() { __kmp_init_lock(&lock); }

  ~kmp_map_lock() { __kmp_destroy_lock(&lock); }

  int acquire(kmp_int32 gtid) { return __kmp_acquire_lock(&lock, gtid); }

  void release(kmp_int32 gtid) { __kmp_release_lock(&lock, gtid); }
};

kmp_map_lock map_lock;

} // namespace

/* -------------------------------------------------------------------------- */
/* Memoization                                                                */
/* -------------------------------------------------------------------------- */

void __kmp_memo_create_cache(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc,
                             kmp_int32 num_vars, kmp_int32 thresh) {

  map_lock.acquire(gtid);
  kmp_memo_cache *cache = map.search(hash_loc);

  if (cache == NULL) {
    cache = static_cast<kmp_memo_cache *>(kmpc_malloc(sizeof(kmp_memo_cache)));
    cache->construct(hash_loc, num_vars, thresh);
    map.insert(cache);
  }
  map_lock.release(gtid);
}

void __kmp_memo_copy_in(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc,
                        void *data_in, size_t size, memo_num_t num_type,
                        kmp_int32 id_var) {
  map_lock.acquire(gtid);
  kmp_memo_cache *cache = map.search(hash_loc);
  KMP_ASSERT2(cache != NULL, "cache not found");

  if (cache->state == UNINITIALIZED)
    cache->insert(id_var, data_in, size, num_type);

  map_lock.release(gtid);
}

kmp_int32 __kmp_memo_verify(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc) {
  map_lock.acquire(gtid);
  kmp_memo_cache *cache = map.search(hash_loc);
  KMP_ASSERT2(cache != NULL, "cache not found");

  switch (cache->state) {
  case UNINITIALIZED:
  case INVALID:
    map_lock.release(gtid);
    return 1;
  case VALID:
    cache->update_address();
    map_lock.release(gtid);
    return 0;
  }

  return 1;
}

void __kmp_memo_compare(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc) {
  map_lock.acquire(gtid);
  kmp_memo_cache *cache = map.search(hash_loc);
  KMP_ASSERT2(cache != NULL, "cache not found");

  if (cache->state == UNINITIALIZED) {
    cache->state = INVALID;
    cache->update_cache();

    map_lock.release(gtid);
    return;
  }

  if (cache->state == INVALID && cache->thresh) {
    cache->verify_validity();
    if (cache->state == INVALID)
      cache->update_cache();

    map_lock.release(gtid);
    return;
  }

  if (cache->state == INVALID && !cache->thresh) {
    cache->update_cache();
    cache->state = VALID;

    map_lock.release(gtid);
    return;
  }

  map_lock.release(gtid);
  KMP_ASSERT2(cache->state == VALID, "invalid cache state");
}

/* ------------------------------------------------------------------------ */
/* Perforation                                                              */
/* ------------------------------------------------------------------------ */

int __kmp_perforation(int gtid, ident_t *id_ref, void *inc_var,
                      perfo_t perfo_type, kmp_int32 induction, kmp_int32 lb,
                      kmp_int32 ub) {
  switch (perfo_type) {
  case perfo_fini: {
    if (*(kmp_int32 *)inc_var + induction > ub)
      *(kmp_int32 *)inc_var += induction;

    return 0;
  }
  case perfo_init: {
    *(kmp_int32 *)inc_var = lb + induction;
    return 0;
  }
  case perfo_small: {
    if (ub == 0)
      return 0;

    srand(1); // Fix seed to use in benchmarks
    kmp_int32 k = (rand() % ((ub - lb) + ub)) % induction;
    if (*(kmp_int32 *)inc_var % induction == k)
      *(kmp_int32 *)inc_var += induction;
    return 0;
  }
  case perfo_large: {
    *(kmp_int32 *)inc_var += induction;
    return 0;
  }
  case perfo_undefined:
  default:
    KMP_ASSERT2(perfo_type >= 0, "undefined perforation type");
  }
  return 0;
}
