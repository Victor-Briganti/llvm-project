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

#include "kmp.h"
#include "kmp_io.h"
#include "kmp_lock.h"
#include "kmp_memo.h"

/* ------------------------------------------------------------------------ */

class kmp_map_t {
  enum bucket_state_t {
    EMPTY = 0,
    OCCUPIED = 1,
    DELETED = 2,
  };

  struct kmp_bucket_t {
    kmp_int32 key;
    kmp_int32 argc;
    bucket_state_t state;
  };

  // Based on the hash algorithm:
  // <http://www.cse.yorku.ca/~oz/hash.html#djb2>
  kmp_int32 hash_func(kmp_int32 key) { return (5381 * 33 + key) % size; }

  void init() {
    entries = 0;
    size = 32;
    buckets = (kmp_bucket_t *)kmpc_calloc(size, sizeof(kmp_bucket_t));
    load_factor = 0.75;
    __kmp_init_lock(&lock);
  }

  void resize(kmp_int32 gtid) {
    kmp_int32 old_size = size;
    kmp_bucket_t *old_buckets = buckets;

    size *= 2;
    entries = 0;
    buckets = (kmp_bucket_t *)kmpc_calloc(size, sizeof(kmp_bucket_t));

    for (kmp_int32 i = 0; i < old_size; i++) {
      if (old_buckets[i].state == OCCUPIED) {
        insert_helper(old_buckets[i].key, old_buckets[i].argc, gtid);
      }
    }

    kmpc_free(old_buckets);
  }

  bool insert_helper(kmp_int32 hashloc, kmp_int32 argc, kmp_int32 gtid) {
    kmp_int32 idx = hash_func(hashloc);
    kmp_int32 start_idx = idx;
    do {
      switch (buckets[idx].state) {
      case DELETED:
      case EMPTY:
        buckets[idx].key = hashloc;
        buckets[idx].argc = argc;
        buckets[idx].state = OCCUPIED;
        entries++;
        return true;
      case OCCUPIED:
        if (buckets[idx].key == hashloc) {
          buckets[idx].argc = argc;
          return true;
        }
        idx = (idx + 1) % size;
      }
    } while (idx != start_idx);

    return false;
  }

public:
  kmp_map_t() = delete;
  kmp_map_t(const kmp_map_t &) = delete;
  kmp_map_t &operator=(const kmp_map_t &) = delete;

  static kmp_map_t *get() {
    __kmp_acquire_bootstrap_lock(&mutex);
    if (!singleton) {
      singleton = (kmp_map_t *)kmpc_malloc(sizeof(kmp_map_t));
      singleton->init();
    }
    __kmp_release_bootstrap_lock(&mutex);

    return singleton;
  }

  void insert(kmp_int32 hashloc, kmp_int32 argc, kmp_int32 gtid) {
    __kmp_acquire_lock(&lock, gtid);
    while (!insert_helper(hashloc, argc, gtid)) {
      if (size >= 4096) {
        __kmp_release_lock(&lock, gtid);
        return;
      }
      resize(gtid);
    }
    __kmp_release_lock(&lock, gtid);
  }

  kmp_int32 get(kmp_int32 hashloc, kmp_int32 gtid) {
    kmp_int32 argc;
    __kmp_acquire_lock(&lock, gtid);

    kmp_int32 idx = hash_func(hashloc);
    kmp_int32 start_idx = idx;
    do {
      switch (buckets[idx].state) {
      case EMPTY:
        argc = 0;
        goto out;
      case OCCUPIED:
        if (buckets[idx].key == hashloc) {
          argc = buckets[idx].argc;
          goto out;
        }
        [[fallthrough]];
      case DELETED:
        idx = (idx + 1) % size;
      }
    } while (idx != start_idx);

  out:
    __kmp_release_lock(&lock, gtid);
    return argc;
  }

private:
  kmp_int32 entries;
  kmp_int32 size;
  kmp_bucket_t *buckets;
  double load_factor;
  kmp_lock_t lock;

  static kmp_bootstrap_lock_t mutex;
  static kmp_map_t *singleton;
};

kmp_map_t *kmp_map_t::singleton = nullptr;
kmp_bootstrap_lock_t kmp_map_t::mutex =
    KMP_BOOTSTRAP_LOCK_INITIALIZER(kmp_map_t::mutex);

/* ------------------------------------------------------------------------ */

int __kmp_memo_in(kmp_int32 hashloc, kmp_int32 argc, kmp_int32 gtid) {
  kmp_map_t *map = kmp_map_t::get();

  __kmp_printf("[%d] memo_in(%d): %d\n", gtid, hashloc, argc);
  map->insert(hashloc, argc, gtid);
  return 1;
}

void __kmp_memo_out(kmp_int32 hashloc, kmp_int32 gtid) {
  kmp_map_t *map = kmp_map_t::get();
  kmp_int32 argc = map->get(hashloc, gtid);
  __kmp_printf("[%d] memo_out(%d): %d\n", gtid, hashloc, argc);
}
