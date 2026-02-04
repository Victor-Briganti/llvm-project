/*
 * kmp_memo.h -- approximate memoization header file.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef KMP_MEMO_H
#define KMP_MEMO_H

#include "kmp.h"

#ifdef __cplusplus
extern "C" {
#endif

int __kmp_memo_in(kmp_int32 hashloc, kmp_int32 argc, kmp_int32 gtid);
void __kmp_memo_out(kmp_int32 hashloc, kmp_int32 gtid);

#ifdef __cplusplus
}
#endif

#endif // KMP_MEMO_H