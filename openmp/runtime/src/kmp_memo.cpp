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
#include "kmp_debug.h"
#include "kmp_lock.h"
#include "kmp_memo.h"

/* ------------------------------------------------------------------------ */
struct kmp_node_t {
  struct kmp_node_t *left;
  struct kmp_node_t *right;
  double *points;
  void *data;
  kmp_int32 n_points;
  memo_num_t dtype;
  kmp_int32 size;
  kmp_int32 n_refs;
};

static size_t memo_dtype_size(memo_num_t dtype) {
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
    return 0;
  }
}

static void memo_compute_mean(kmp_node_t **nodes, kmp_int32 count,
                              memo_num_t dtype, void *result) {
  if (count == 0)
    return;

  double sum = 0.0;
  for (kmp_int32 i = 0; i < count; i++) {
    switch (dtype) {
    case memo_num_bool:
      sum += *((bool *)nodes[i]->data) ? 1.0 : 0.0;
      break;
    case memo_num_char:
      sum += (double)*((char *)nodes[i]->data);
      break;
    case memo_num_uchar:
      sum += (double)*((unsigned char *)nodes[i]->data);
      break;
    case memo_num_short:
      sum += (double)*((short *)nodes[i]->data);
      break;
    case memo_num_ushort:
      sum += (double)*((unsigned short *)nodes[i]->data);
      break;
    case memo_num_int:
      sum += (double)*((int *)nodes[i]->data);
      break;
    case memo_num_uint:
      sum += (double)*((unsigned int *)nodes[i]->data);
      break;
    case memo_num_long:
      sum += (double)*((long *)nodes[i]->data);
      break;
    case memo_num_ulong:
      sum += (double)*((unsigned long *)nodes[i]->data);
      break;
    case memo_num_longlong:
      sum += (double)*((long long *)nodes[i]->data);
      break;
    case memo_num_ulonglong:
      sum += (double)*((unsigned long long *)nodes[i]->data);
      break;
    case memo_num_float:
      sum += (double)*((float *)nodes[i]->data);
      break;
    case memo_num_double:
      sum += *((double *)nodes[i]->data);
      break;
    case memo_num_longdouble:
      sum += (double)*((long double *)nodes[i]->data);
      break;
    default:
      break;
    }
  }

  double mean = sum / (double)count;

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
    break;
  }
}

class kmp_kd_tree_t {
  kmp_node_t *node_create(double *points, int n_points, void *data,
                          memo_num_t dtype) {
    kmp_node_t *node = (kmp_node_t *)kmpc_malloc(sizeof(kmp_node_t));
    node->n_points = n_points;
    node->points = (double *)kmpc_malloc(n_points * sizeof(double));
    memcpy(node->points, points, n_points * sizeof(double));
    node->left = nullptr;
    node->right = nullptr;
    size_t data_size = memo_dtype_size(dtype);
    node->data = kmpc_malloc(data_size);
    memcpy(node->data, data, data_size);
    node->dtype = dtype;
    node->size = 1;
    node->n_refs = 0;
    return node;
  }

  static int node_compare(const void *a, const void *b) {
    kmp_node_t *node_a = *(kmp_node_t **)a;
    kmp_node_t *node_b = *(kmp_node_t **)b;
    double diff = node_a->points[g_axis] - node_b->points[g_axis];
    return (diff < 0) ? -1 : (diff > 0) ? 1 : 0;
  }

  kmp_int32 node_size(const kmp_node_t *node) {
    if (!node)
      return 0;

    return node->size;
  }

  void node_update_size(kmp_node_t *node) {
    node->size = 1 + node_size(node->left) + node_size(node->right);
  }

  bool node_is_unbalanced(kmp_node_t *node) {
    double threshold = (double)node->size * alpha;
    return (node_size(node->left) > threshold) ||
           (node_size(node->right) > threshold);
  }

  double point_distance(const double *a, const double *b) {
    double diff = 0;

    for (int i = 0; i < k; i++) {
      diff += (a[i] - b[i]) * (a[i] - b[i]);
    }

    return diff;
  }

  kmp_int32 flatten(kmp_node_t **node_list, kmp_node_t *head, int max_nodes) {
    KMP_ASSERT2(node_list, "Node list should not be NULL");
    KMP_ASSERT2(head, "Head should not be NULL");

    int top = 0, index = 0;
    node_list[top++] = head;

    while (index < top && top < max_nodes) {
      kmp_node_t *current = node_list[index];

      if (current->left != NULL) {
        node_list[top++] = current->left;
      }

      if (current->right != NULL) {
        node_list[top++] = current->right;
      }

      node_list[index]->left = NULL;
      node_list[index]->right = NULL;
      index++;
    }

    return top;
  }

  kmp_node_t *rebuild_r(kmp_node_t **node_list, int num_nodes, int depth) {
    if (node_list == nullptr || !num_nodes) {
      return nullptr;
    }

    g_axis = depth % k;
    qsort(node_list, (size_t)num_nodes, sizeof(kmp_node_t *), node_compare);

    int median_idx = num_nodes / 2;
    kmp_node_t *head = node_list[median_idx];

    head->left = rebuild_r(&node_list[0], median_idx, depth + 1);
    head->right = rebuild_r(&node_list[median_idx + 1],
                            num_nodes - median_idx - 1, depth + 1);

    node_update_size(head);
    return head;
  }

  kmp_node_t *insert_r(kmp_node_t *node, kmp_node_t *new_node,
                       kmp_int32 radius_sq, kmp_int32 depth, bool &rebuild) {
    if (!node) {
      rebuild = false;
      return new_node;
    }

    double diff = point_distance(node->points, new_node->points);
    if (diff <= radius_sq) {
      node->n_refs++;
    }

    int axis = depth % k;
    if (new_node->points[axis] < node->points[axis]) {
      node->left =
          insert_r(node->left, new_node, radius_sq, depth + 1, rebuild);
    } else {
      node->right =
          insert_r(node->right, new_node, radius_sq, depth + 1, rebuild);
    }

    node_update_size(node);
    if (!rebuild && node_is_unbalanced(node)) {
      kmp_node_t **node_list =
          (kmp_node_t **)kmpc_malloc(num_leafs * sizeof(kmp_node_t *));
      kmp_int32 n_nodes = flatten(node_list, node, num_leafs);
      node = rebuild_r(node_list, n_nodes, depth);
      kmpc_free(node_list);
    }

    return node;
  }

  void radius_search_r(const double *points, int num_points, kmp_node_t *node,
                       kmp_int32 radius_sq, kmp_node_t **node_list,
                       kmp_int32 max_nodes, kmp_int32 &idx, kmp_int32 depth) {
    if (node == NULL || idx == max_nodes) {
      return;
    }

    double distance = point_distance(node->points, points);
    if (distance < 0) {
      return;
    }

    if (distance <= radius_sq) {
      node_list[idx] = node;
      idx++;

      if (idx == max_nodes) {
        return;
      }
    }

    int axis = depth % k;
    double diff = points[axis] - node->points[axis];

    kmp_node_t *near_child = diff < 0 ? node->left : node->right;
    kmp_node_t *far_child = diff < 0 ? node->right : node->left;

    radius_search_r(points, num_points, near_child, radius_sq, node_list,
                    max_nodes, idx, depth + 1);

    if ((diff * diff) <= radius_sq) {
      radius_search_r(points, num_points, far_child, radius_sq, node_list,
                      max_nodes, idx, depth + 1);
    }
  }

public:
  void init(kmp_int32 dims) {
    root = nullptr;
    num_leafs = 0;
    k = dims;
    alpha = 0.7;
    __kmp_init_lock(&lock);
  }

  bool is_empty() { return root == nullptr; }

  kmp_int32 get_k() { return k; }

  void insert(double *points, int n_points, void *data, memo_num_t dtype,
              kmp_int32 radius, kmp_int32 gtid) {
    KMP_ASSERT2(points, "Inserted points should not be NULL");
    KMP_ASSERT2(data, "Data should not be NULL");

    kmp_node_t *node = node_create(points, n_points, data, dtype);
    KMP_ASSERT2(node, "Could not allocate node");

    __kmp_acquire_lock(&lock, gtid);
    bool rebuild = false;
    root = insert_r(root, node, radius * radius, 0, rebuild);
    num_leafs++;
    __kmp_release_lock(&lock, gtid);
  }

  kmp_node_t *search_max_ref(double *points, int n_points, kmp_int32 radius,
                             kmp_int32 gtid) {
    KMP_ASSERT2(points, "Search points should not be NULL");

    __kmp_acquire_lock(&lock, gtid);
    kmp_node_t *head = root;
    kmp_node_t *max_node = nullptr;
    kmp_int32 dept = 0;
    while (head != nullptr) {
      kmp_int32 axis = dept % k;

      kmp_int32 diff = point_distance(head->points, points);
      if (diff <= radius * radius) {
        if (max_node == nullptr || head->n_refs > max_node->n_refs) {
          max_node = head;
        }
      }

      if (points[axis] < head->points[axis]) {
        head = head->left;
      } else {
        head = head->right;
      }
      dept++;
    }

    __kmp_release_lock(&lock, gtid);
    return max_node;
  }

  int kdtree_radius_search(const double *point, int num_points, double radius,
                           kmp_node_t **node_list, int max_nodes,
                           kmp_int32 gtid) {
    kmp_int32 idx = 0;
    __kmp_acquire_lock(&lock, gtid);
    radius_search_r(point, num_points, root, radius * radius, node_list,
                    max_nodes, idx, 0);
    __kmp_release_lock(&lock, gtid);
    return idx;
  }

private:
  kmp_node_t *root;
  kmp_int32 num_leafs;
  kmp_int32 k;
  double alpha;
  kmp_lock_t lock;

  static kmp_int32 g_axis;
};

kmp_int32 kmp_kd_tree_t::g_axis = 0;

/* ------------------------------------------------------------------------ */

class kmp_map_t {
  enum bucket_state_t {
    EMPTY = 0,
    OCCUPIED = 1,
    DELETED = 2,
  };

  struct kmp_bucket_t {
    kmp_int32 key;
    kmp_kd_tree_t *tree;
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
        insert_helper(old_buckets[i].key, old_buckets[i].tree, gtid);
      }
    }

    kmpc_free(old_buckets);
  }

  bool insert_helper(kmp_int32 hashloc, kmp_kd_tree_t *tree, kmp_int32 gtid) {
    kmp_int32 idx = hash_func(hashloc);
    kmp_int32 start_idx = idx;
    do {
      switch (buckets[idx].state) {
      case DELETED:
      case EMPTY:
        buckets[idx].key = hashloc;
        buckets[idx].tree = tree;
        buckets[idx].state = OCCUPIED;
        entries++;
        return true;
      case OCCUPIED:
        if (buckets[idx].key == hashloc) {
          buckets[idx].tree = tree;
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

  kmp_kd_tree_t *get_or_create(kmp_int32 hashloc, kmp_int32 dims,
                               kmp_int32 gtid) {
    __kmp_acquire_lock(&lock, gtid);

    kmp_int32 idx = hash_func(hashloc);
    kmp_int32 start_idx = idx;
    do {
      switch (buckets[idx].state) {
      case EMPTY:
        goto create_new;
      case OCCUPIED:
        if (buckets[idx].key == hashloc) {
          kmp_kd_tree_t *tree = buckets[idx].tree;
          __kmp_release_lock(&lock, gtid);
          return tree;
        }
        [[fallthrough]];
      case DELETED:
        idx = (idx + 1) % size;
      }
    } while (idx != start_idx);

  create_new:
    kmp_kd_tree_t *new_tree =
        (kmp_kd_tree_t *)kmpc_malloc(sizeof(kmp_kd_tree_t));
    new_tree->init(dims);

    while (!insert_helper(hashloc, new_tree, gtid)) {
      if (size >= 4096) {
        __kmp_release_lock(&lock, gtid);
        return new_tree;
      }
      resize(gtid);
    }

    __kmp_release_lock(&lock, gtid);
    return new_tree;
  }

  kmp_kd_tree_t *get(kmp_int32 hashloc, kmp_int32 gtid) {
    kmp_kd_tree_t *tree = nullptr;
    __kmp_acquire_lock(&lock, gtid);

    kmp_int32 idx = hash_func(hashloc);
    kmp_int32 start_idx = idx;
    do {
      switch (buckets[idx].state) {
      case EMPTY:
        goto out;
      case OCCUPIED:
        if (buckets[idx].key == hashloc) {
          tree = buckets[idx].tree;
          goto out;
        }
        [[fallthrough]];
      case DELETED:
        idx = (idx + 1) % size;
      }
    } while (idx != start_idx);

  out:
    __kmp_release_lock(&lock, gtid);
    return tree;
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

int __kmp_memo_in(kmp_int32 hashloc, double *points, kmp_int32 n_points,
                  void *data, memo_num_t dtype, kmp_int32 radius,
                  kmp_int32 gtid) {
  kmp_map_t *map = kmp_map_t::get();

  kmp_kd_tree_t *tree = map->get_or_create(hashloc, n_points, gtid);
  KMP_ASSERT2(tree, "Could not get or create tree");

  if (tree->is_empty()) {
    return 1;
  }

  kmp_node_t *max_node = tree->search_max_ref(points, n_points, radius, gtid);

  if (max_node == nullptr) {
    return 1;
  }

  // Check if n_refs >= 2^k (threshold)
  kmp_int32 k = tree->get_k();
  kmp_int32 threshold = 1 << k;

  if (max_node->n_refs >= threshold) {
    const kmp_int32 max_nodes = 64;
    kmp_node_t **node_list =
        (kmp_node_t **)kmpc_malloc(max_nodes * sizeof(kmp_node_t *));

    kmp_int32 count =
        tree->kdtree_radius_search(points, n_points, radius, node_list,
                                   max_nodes, gtid);

    if (count > 0) {
      memo_compute_mean(node_list, count, dtype, data);
    }

    kmpc_free(node_list);
    return 0;
  }
  return 1;
}

void __kmp_memo_out(kmp_int32 hashloc, double *points, kmp_int32 n_points,
                    void *data, memo_num_t dtype, kmp_int32 radius,
                    kmp_int32 gtid) {
  kmp_map_t *map = kmp_map_t::get();

  kmp_kd_tree_t *tree = map->get(hashloc, gtid);
  KMP_ASSERT2(tree, "Tree should exist in memo_out");
  tree->insert(points, n_points, data, dtype, radius, gtid);
}
