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

#include "kmp_approx.h"
#include "kmp.h"
#include <math.h>

/* ------------------------------------------------------------------------ */
/* Memoization                                                              */
/* ------------------------------------------------------------------------ */

// TODO: Enable read/write locks on the structure. Following:
// https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock#Using_two_mutexes
kmp_memo_map kmap;

void kmp_memo_cache::construct(kmp_int32 location, kmp_int32 num_vars,
                               kmp_real64 threshold) {
  loc = location;
  nvars = num_vars;
  sizes = (size_t *)kmpc_malloc(sizeof(size_t) * num_vars);
  types = (memo_num_t *)kmpc_malloc(sizeof(memo_num_t) * num_vars);
  types = (memo_num_t *)kmpc_malloc(sizeof(memo_num_t) * num_vars);
  datas = (void **)kmpc_malloc(sizeof(void *) * num_vars);
  addresses = (void **)kmpc_malloc(sizeof(void *) * num_vars);

  for (kmp_int32 i = 0; i < num_vars; i++) {
    sizes[i] = 0;
    types[i] = memo_num_undefined;
    datas[i] = NULL;
    addresses[i] = NULL;
  }

  thresh = threshold <= 0 ? 0 : threshold;
  valid = threshold <= 0 ? INVALID : UNINITIALIZED;
}

void kmp_memo_cache::insert(kmp_int32 idx, void *var, size_t size, memo_num_t type) {
  types[idx] = type;
  sizes[idx] = size;
  datas[idx] = kmpc_malloc(size);
  datas[idx] = NULL;
  addresses[idx] = var;
}

void kmp_memo_cache::update_cache() {
  for (kmp_int32 i = 0; i < nvars; i++)
    memcpy(datas[i], addresses[i], sizes[i]);
}

void kmp_memo_cache::update_address() {
  for (kmp_int32 i = 0; i < nvars; i++)
    memcpy(addresses[i], datas[i], sizes[i]);
}

void kmp_memo_cache::destruct() {
  kmpc_free(types);
  kmpc_free(sizes);
  for (kmp_int32 i = 0; i < nvars; i++)
    kmpc_free(datas[i]);
  kmpc_free(datas);
}

/*----------------------------------------------------------------------------*/

kmp_memo_map::kmp_memo_map() {
  buckets = (kmp_memo_cache **)kmpc_malloc(KMP_MAP_INIT_SIZE *
                                           sizeof(kmp_memo_cache *));
  for (kmp_int32 i = 0; i < KMP_MAP_INIT_SIZE; i++)
    buckets[i] = NULL;

  nbuckets = KMP_MAP_INIT_SIZE;
  entries = 0;
}

kmp_memo_map::~kmp_memo_map() {
  for (kmp_int32 i = 0; i < nbuckets; i++) {
    if (this->buckets[i] != NULL) {
      this->buckets[i]->destruct();
      kmpc_free(this->buckets[i]);
    }
  }

  kmpc_free(this->buckets);
}

// This hash implementation is based on the sdbm algorithm.
// More about it can be found:
// http://www.cse.yorku.ca/~oz/hash.html#sdbm
// https://github.com/davidar/sdbm/blob/29d5ed2b5297e51125ee45f6efc5541851aab0fb/hash.c#L18-L47
kmp_int32 kmp_memo_map::bucket_index(kmp_int32 loc) {
  kmp_int32 hash = 0;
  hash = loc + (loc << 6) + (hash << 16) - loc;
  return (hash % this->nbuckets);
}

void kmp_memo_map::rehash() {
  kmp_memo_cache **old_buckets = buckets;
  kmp_int32 old_nbuckets = nbuckets;

  nbuckets *= 2;
  entries = 0;

  buckets =
      (kmp_memo_cache **)kmpc_malloc(sizeof(kmp_memo_cache *) * nbuckets * 2);
  for (kmp_int32 i = 0; i < nbuckets; i++)
    buckets[i] = NULL;

  for (kmp_int32 i = 0; i < old_nbuckets; i++) {
    if (old_buckets[i] != NULL) {
      insert(old_buckets[i]);
    }
  }

  kmpc_free(old_buckets);
}

void kmp_memo_map::insert(kmp_memo_cache *cache) {
  kmp_int32 idx = bucket_index(cache->loc);

  kmp_int32 aux_idx = idx;
  while (buckets[aux_idx] != NULL) {
    aux_idx = (aux_idx + 1) % nbuckets;
    if (aux_idx == idx) {
      rehash();
      insert(cache);
      return;
    }
  }

  buckets[aux_idx] = cache;
  entries++;

  auto placeholder = ((float)entries / (float)nbuckets);
  if (placeholder >= KMP_MAP_LOAD)
    rehash();
}

kmp_memo_cache *kmp_memo_map::search(kmp_int32 loc) {
  kmp_int32 idx = bucket_index(loc);
  if (buckets[idx] == NULL)
    return NULL;

  if (buckets[idx]->loc == loc)
    return buckets[idx];

  kmp_int32 i = (idx + 1) % nbuckets;
  while (buckets[i] != NULL && i != idx) {
    if (buckets[i]->loc == loc)
      return buckets[i];

    i = (i + 1) % nbuckets;
  }

  return NULL;
}

/*----------------------------------------------------------------------------*/

static kmp_real64 convert(void *value, memo_num_t type) {
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
  default:
    KMP_ASSERT(0 && "Invalid number type");
  }
}

// Percentage difference formula:
// fabs(x - y) / y
static void compare(kmp_memo_cache *cache) {
  kmp_real64 final = 0.0f;
  kmp_real64 res[cache->nvars];

  kmp_int32 i;
  for (i = 0; i < cache->nvars; i++) {
    kmp_real64 data = convert( cache->datas[i], cache->types[i]); 
    kmp_real64 address = convert( cache->addresses[i], cache->types[i]); 
    res[i] = fabs(address - data) / data;
  }

  for (i = 0; i < cache->nvars; i++)
    final += res[i];

  final = final / (kmp_real64)cache->nvars;
  cache->valid = (cache_state)islessequal(final, cache->thresh);
}

/*----------------------------------------------------------------------------*/

void __kmp_memo_create_cache(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc,
                             kmp_int32 num_vars, kmp_int32 tresh) {
  kmp_memo_cache *cache = kmap.search(hash_loc);
  if (cache != NULL)
    return;

  cache = (kmp_memo_cache *)kmpc_malloc(sizeof(kmp_memo_cache));
  cache->construct(hash_loc, num_vars, tresh);
  kmap.insert(cache);
}

void __kmp_memo_copy_in(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc,
                        void *data, size_t size, memo_num_t num_type,
                        kmp_int32 id_var) {
  kmp_memo_cache *cache = kmap.search(hash_loc);
  if (cache->valid == UNINITIALIZED)
    cache->insert(id_var, data, size, num_type);
}

kmp_int32 __kmp_memo_verify(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc) {
  kmp_memo_cache *cache = kmap.search(hash_loc);
  switch (cache->valid) {
  case UNINITIALIZED:
  case INVALID:
    return 1;
  case VALID:
    cache->update_address();
  }
  return 0;
}

void __kmp_memo_compare(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc) {
  kmp_memo_cache *cache = kmap.search(hash_loc);
  if (cache->valid == UNINITIALIZED) {
    cache->valid = INVALID;
    cache->update_cache();
    return;
  }

  if (cache->valid == INVALID && cache->thresh) {
    compare(cache);
    if (cache->valid == VALID)
      cache->update_cache();
  }

  if (cache->valid == INVALID && !cache->thresh) {
    cache->update_cache();
    cache->valid = VALID;
  }
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