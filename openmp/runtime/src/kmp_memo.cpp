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

/* ------------------------------------------------------------------------ */

int __kmp_memo_in(kmp_int32 hashloc, kmp_int32 gtid, kmp_int32 radius,
                  void *data, memo_num_t dtype) {
  return 1;
}

void __kmp_memo_out(kmp_int32 hashloc, kmp_int32 gtid, kmp_int32 radius,
                    void *data, memo_num_t dtype) {
}
