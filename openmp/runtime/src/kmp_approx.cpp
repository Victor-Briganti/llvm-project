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
#include <algorithm>

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
/* Hashmap                                                                    */
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
    KMP_ASSERT2(this->sizes != nullptr, "fail to allocate");
    this->types = static_cast<memo_num_t *>(
        kmpc_malloc(sizeof(memo_num_t) * this->nvars));
    KMP_ASSERT2(this->types != nullptr, "fail to allocate");
    this->datas =
        static_cast<void **>(kmpc_malloc(sizeof(void *) * this->nvars));
    KMP_ASSERT2(this->datas != nullptr, "fail to allocate");
    this->addresses =
        static_cast<void **>(kmpc_malloc(sizeof(void *) * this->nvars));
    KMP_ASSERT2(this->addresses != nullptr, "fail to allocate");

    for (kmp_int32 i = 0; i < nvars; i++) {
      this->sizes[i] = 0;
      this->types[i] = memo_num_undefined;
      this->datas[i] = nullptr;
      this->addresses[i] = nullptr;
    }

    if (threshold) {
      this->thresh = threshold / 100;
    } else {
      this->thresh = 0;
    }

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
    sizes = nullptr;
    types = nullptr;
    datas = nullptr;
    addresses = nullptr;
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
    kmp_real64 final = 0.0;
    kmp_real64 *res =
        static_cast<kmp_real64 *>(kmpc_malloc(sizeof(kmp_real64) * nvars));
    KMP_ASSERT2(res != nullptr, "fail to allocate");

    for (kmp_int32 i = 0; i < nvars; i++) {
      kmp_real64 data = convert(datas[i], types[i]);
      kmp_real64 address = convert(addresses[i], types[i]);

      if (!data) {
        res[i] = 0.0;
      } else {
        res[i] = fabs(address - data) / data;
      }
    }

    for (kmp_int32 i = 0; i < nvars; i++)
      final += res[i];

    final = final / (kmp_real64)nvars;

    if (!final) {
      state = INVALID;
    } else {
      state = (cache_state)islessequal(final, thresh);
    }
  }
};

/*----------------------------------------------------------------------------*/

class kmp_memo_map {
  kmp_memo_cache **buckets = nullptr;
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
    KMP_ASSERT2(buckets == nullptr, "fail to allocate");

    for (kmp_int32 i = 0; i < nbuckets; i++)
      buckets[i] = nullptr;

    for (kmp_int32 i = 0; i < old_nbuckets; i++) {
      if (old_buckets[i] != nullptr) {
        insert(old_buckets[i]);
      }
    }

    kmpc_free(old_buckets);
  }

public:
  void construct() {
    nbuckets = KMP_MAP_INIT_SIZE;
    entries = 0;

    size_t bucket_size = sizeof(kmp_memo_cache *) * KMP_MAP_INIT_SIZE;
    buckets = static_cast<kmp_memo_cache **>(kmpc_malloc(bucket_size));
    KMP_ASSERT2(buckets != nullptr, "fail to allocate");

    for (kmp_int32 i = 0; i < KMP_MAP_INIT_SIZE; i++) {
      buckets[i] = nullptr;
    }
  }

  void insert(kmp_memo_cache *cache) {
    kmp_int32 idx = bucket_index(cache->loc);
    kmp_int32 aidx = idx;

    while (buckets[aidx] != nullptr) {
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
    while (cache != nullptr) {
      if (cache->loc == loc)
        return cache;

      idx = (idx + 1) % nbuckets;
      cache = buckets[idx];
    }

    return nullptr;
  }
};

/*----------------------------------------------------------------------------*/
/* AVL                                                                        */
/*----------------------------------------------------------------------------*/

class kmp_memo_node {
  kmp_int32 height;

public:
  kmp_int32 key;
  kmp_memo_map *data;
  kmp_memo_node *parent;
  kmp_memo_node *left;
  kmp_memo_node *right;

  void construct(kmp_int32 key, kmp_memo_map *data) {
    this->key = key;
    this->data = data;
    this->height = 1;
    this->parent = nullptr;
    this->left = nullptr;
    this->right = nullptr;
  }

  inline kmp_int32 max(kmp_int32 x, kmp_int32 y) { return x > y ? x : y; }

  kmp_int32 get_height(kmp_memo_node *node) {
    if (node == nullptr)
      return 0;

    return node->height;
  }

  kmp_int32 load_balance(kmp_memo_node *node) {
    if (node == nullptr) {
      return 0;
    }

    return get_height(node->left) - get_height(node->right);
  }

  void update_height() {
    height = 1 + max(get_height(left), get_height(right));
  }

  bool insert_child(kmp_int32 key, kmp_memo_map *data) {
    if (this->key > key && this->left == nullptr) {
      this->left =
          static_cast<kmp_memo_node *>(kmpc_malloc(sizeof(kmp_memo_node)));
      this->left->construct(key, data);
      this->left->parent = this;
      return true;
    }

    if (this->key < key && this->right == nullptr) {
      this->right =
          static_cast<kmp_memo_node *>(kmpc_malloc(sizeof(kmp_memo_node)));
      this->right->construct(key, data);
      this->right->parent = this;
      return true;
    }

    return false;
  }

  void parent_update(kmp_memo_node *node, kmp_memo_node *parent) {
    if (node == nullptr)
      return;

    node->parent = parent;
  }

  /* Perform a rotation of the node to the left side.
   *
   *        root                       pivot
   *        /   \                      /   \
   *    subroot pivot      =>        root   X
   *             /  \                /  \
   *            /    \              /    \
   *        subpivot  X         subroot subpivot
   */
  void rotate_left(kmp_memo_node *node) {
    kmp_memo_node *root = node;
    kmp_memo_node *pivot = root->right;
    kmp_memo_node *subRoot = root->left;
    kmp_memo_node *subPivot = pivot->left;

    // To perform the rotation between the pivot and the root, only the key and
    // the data are swapped. In this way it's not necessary to change the
    // parent's node of the root.
    std::swap(root->key, pivot->key);
    std::swap(root->data, pivot->data);

    root->right = pivot->right;

    pivot->right = subPivot;
    pivot->left = subRoot;

    root->left = pivot;

    parent_update(subRoot, pivot);
    parent_update(root->right, root);

    pivot->update_height();
    root->update_height();
  }

  /* Perform a rotation of the node to the right side.
   *
   *       root                 pivot
   *       /   \                /   \
   *    pivot  subtroot  =>    X   root
   *     / \                        / \
   *    /   \                      /   \
   *   X  subpivot            subpivot subtroot
   */
  void rotate_right(kmp_memo_node *node) {
    kmp_memo_node *root = node;
    kmp_memo_node *pivot = root->left;
    kmp_memo_node *subRoot = root->right;
    kmp_memo_node *subPivot = pivot->right;

    // To perform the rotation between the pivot and the root, only the key and
    // the data are swapped. In this way it's not necessary to change the
    // parent's node of the root.
    std::swap(root->key, pivot->key);
    std::swap(root->data, pivot->data);

    root->left = pivot->left;

    pivot->left = subPivot;
    pivot->right = subRoot;

    root->right = pivot;

    parent_update(subRoot, pivot);
    parent_update(root->left, root);

    pivot->update_height();
    root->update_height();
  }

  /* Handles the growth of the tree after a insertion. There are four cases that
   * make the need to rebalance the tree:
   *
   * 1. Left-Left Case:
   *        z                                      y
   *       / \                                   /   \
   *      y   T4      Right Rotate (z)          x      z
   *     / \          - - - - - - - - ->      /  \    /  \
   *    x   T3                               T1  T2  T3  T4
   *   / \
   * T1   T2
   *
   * 2. Left-Right Case:
   *     z                               z                           x
   *    / \                            /   \                        /  \
   *   y   T4  Left Rotate (y)        x    T4  Right Rotate(z)    y      z
   *  / \      - - - - - - - - ->    /  \      - - - - - - - ->  / \    / \
   * T1  x                          y    T3                     T1 T2  T3  T4
   *    / \                        / \
   *  T2   T3                    T1   T2
   *
   * 3. Right-Right Case:
   *    z                               y
   *  /  \                            /   \
   * T1   y     Left Rotate(z)      z      x
   *    /  \   - - - - - - - ->    / \    / \
   *   T2   x                     T1  T2 T3  T4
   *       / \
   *     T3  T4
   *
   * 4. Right-Left Case:
   *    z                            z                            x
   *   / \                          / \                          /  \
   * T1   y   Right Rotate (y)    T1   x      Left Rotate(z)   z      y
   *     / \  - - - - - - - - ->     /  \   - - - - - - - ->  / \    / \
   *    x   T4                      T2   y                  T1  T2  T3  T4
   *   / \                              /  \
   * T2   T3                           T3   T4
   *
   */
  void rebalance(kmp_int32 key) {
    kmp_memo_node *node = this;
    do {
      node->update_height();
      int balance = load_balance(node);

      // 1. Left-Left case
      if (balance > 1 && node->left->key > key) {
        rotate_right(node);
        break;
      }

      // 2. Left-Right case
      if (balance > 1 && node->left->key < key) {
        rotate_left(node->left);
        parent_update(node->left, node);
        rotate_right(node);
        break;
      }

      // 3. Right-Right case
      if (balance < -1 && node->right->key < key) {
        rotate_left(node);
        break;
      }

      // 4. Right-Left case
      if (balance < -1 && node->right->key > key) {
        rotate_right(node->right);
        parent_update(node->right, node);
        rotate_left(node);
        break;
      }

      node = node->parent;
    } while (node != nullptr);
  }
};

class kmp_memo_tree {
  kmp_memo_node *head;
  kmp_int32 size;

public:
  kmp_memo_tree() {
    head = nullptr;
    size = 0;
  }

  kmp_memo_map *search(kmp_int32 key) {
    kmp_memo_node *iter = head;

    while (iter != nullptr) {
      if (iter->key == key) {
        return iter->data;
      }

      if (iter->key < key)
        iter = iter->right;
      else
        iter = iter->left;
    }

    return nullptr;
  }

  bool insert(kmp_int32 key, kmp_memo_map *data) {
    if (head == nullptr) {
      head = static_cast<kmp_memo_node *>(kmpc_malloc(sizeof(kmp_memo_node)));
      head->construct(key, data);
      size++;
      return true;
    }

    kmp_memo_node *iter = head;
    while (true) {
      if (iter->key == key)
        return false;

      if (iter->insert_child(key, data))
        break;

      if (iter->key > key)
        iter = iter->left;
      else
        iter = iter->right;
    }

    iter->rebalance(key);
    size++;
    return true;
  }
};

kmp_memo_tree global_map;

/* -------------------------------------------------------------------------- */
/* Lock                                                                       */
/* -------------------------------------------------------------------------- */

class kmp_memo_lock {
  kmp_lock_t lock;

public:
  kmp_memo_lock() { __kmp_init_lock(&lock); }

  ~kmp_memo_lock() { __kmp_destroy_lock(&lock); }

  int acquire(kmp_int32 gtid) { return __kmp_acquire_lock(&lock, gtid); }

  void release(kmp_int32 gtid) { __kmp_release_lock(&lock, gtid); }
};

kmp_memo_lock memo_lock;

} // namespace

/* -------------------------------------------------------------------------- */
/* Memoization                                                                */
/* -------------------------------------------------------------------------- */

void __kmp_memo_create_cache(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc,
                             kmp_int32 num_vars, kmp_int32 thresh) {
  memo_lock.acquire(gtid);
  kmp_memo_map *map = global_map.search(gtid);
  if (map == nullptr) {
    map = static_cast<kmp_memo_map *>(kmpc_malloc(sizeof(kmp_memo_map)));
    KMP_ASSERT2(map != nullptr, "cache not found");
    map->construct();
    if (!global_map.insert(gtid, map)) {
      KMP_ASSERT2(false, "duplicate key");
    }
  }
  memo_lock.release(gtid);

  kmp_memo_cache *cache = map->search(hash_loc);
  if (cache == nullptr) {
    cache = static_cast<kmp_memo_cache *>(kmpc_malloc(sizeof(kmp_memo_cache)));
    cache->construct(hash_loc, num_vars, thresh);
    map->insert(cache);
  }
}

void __kmp_memo_copy_in(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc,
                        void *data_in, size_t size, memo_num_t num_type,
                        kmp_int32 id_var) {
  memo_lock.acquire(gtid);
  kmp_memo_map *map = global_map.search(gtid);
  if (map == nullptr) {
    map = static_cast<kmp_memo_map *>(kmpc_malloc(sizeof(kmp_memo_map)));
    KMP_ASSERT2(map != nullptr, "cache not found");
    map->construct();
    if (!global_map.insert(gtid, map)) {
      KMP_ASSERT2(false, "duplicate key");
    }
  }
  memo_lock.release(gtid);

  kmp_memo_cache *cache = map->search(hash_loc);
  KMP_ASSERT2(cache != nullptr, "cache not found");

  if (cache->state == UNINITIALIZED)
    cache->insert(id_var, data_in, size, num_type);
}

kmp_int32 __kmp_memo_verify(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc) {
  memo_lock.acquire(gtid);
  kmp_memo_map *map = global_map.search(gtid);
  if (map == nullptr) {
    map = static_cast<kmp_memo_map *>(kmpc_malloc(sizeof(kmp_memo_map)));
    KMP_ASSERT2(map != nullptr, "cache not found");
    map->construct();
    if (!global_map.insert(gtid, map)) {
      KMP_ASSERT2(false, "duplicate key");
    }
  }
  memo_lock.release(gtid);

  kmp_memo_cache *cache = map->search(hash_loc);
  KMP_ASSERT2(cache != nullptr, "cache not found");

  switch (cache->state) {
  case UNINITIALIZED:
  case INVALID:
    return 1;
  case VALID:
    cache->update_address();
    return 0;
  }

  return 1;
}

void __kmp_memo_compare(kmp_int32 gtid, ident_t *loc, kmp_int32 hash_loc) {
  memo_lock.acquire(gtid);
  kmp_memo_map *map = global_map.search(gtid);
  if (map == nullptr) {
    map = static_cast<kmp_memo_map *>(kmpc_malloc(sizeof(kmp_memo_map)));
    KMP_ASSERT2(map != nullptr, "cache not found");
    map->construct();
    if (!global_map.insert(gtid, map)) {
      KMP_ASSERT2(false, "duplicate key");
    }
  }
  memo_lock.release(gtid);

  kmp_memo_cache *cache = map->search(hash_loc);
  KMP_ASSERT2(cache != nullptr, "cache not found");

  if (cache->state == UNINITIALIZED) {
    cache->state = INVALID;
    cache->update_cache();
    return;
  }

  if (cache->state == INVALID && cache->thresh) {
    cache->verify_validity();
    if (cache->state == INVALID)
      cache->update_cache();

    return;
  }

  if (cache->state == INVALID && !cache->thresh) {
    cache->update_cache();
    cache->state = VALID;
    return;
  }

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
