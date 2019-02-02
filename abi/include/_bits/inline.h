/*
 * Copyright (c) 2017 CZ.NIC, z.s.p.o.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/** @addtogroup bits
 * @{
 */
/** @file
 *
 * Portable, compiler-version-agnostic inlining.
 *
 * Usage:
 *
 * In a header file, define a function as:
 * ```
 * __inline int somefunc(int arg1, int arg2)
 * {
 * 	// Function body.
 * }
 * ```
 *
 * Then, in exactly one C file (of your library), do this:
 * ```
 * #include <somefunc_header.h>
 *
 * // Emit code for any non-inlined invocations of this function.
 * __emit_inline int somefunc(int, int);
 * ```
 *
 * Why not just use `static inline`?
 * With `static inline`, each file that can't inline a call to the function
 * will instantiate its own copy of it, resulting in more code.
 * Also, taking a pointer of such function will result in different
 * addresses in different files, which is not obvious.
 *
 */

#ifndef _BITS_INLINE_H_
#define _BITS_INLINE_H_

#undef __inline
#undef __emit_inline

#if defined(__cplusplus)

/* C++ */
#define __inline      inline
#undef  __emit_inline

#elif (defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__)) || (defined(__clang__) && !(__STDC_VERSION__ >= 199901L))

/* Old GNU semantics with GCC alternate keyword. */
#define __inline       extern __inline__
#define __emit_inline  __inline__

#elif defined(__GNUC__) && defined(__GNUC_STDC_INLINE__)

/* C99 semantics with GCC alternate keyword. */
#define __inline       __inline__
#define __emit_inline  extern __inline__

#elif __STDC_VERSION__ >= 199901L

/* C99 semantics with standard keyword. */
#define __inline       inline
#define __emit_inline  extern inline

#else

/* Assume no inline support. */
#define __inline       static
#define __emit_inline  static

#endif

#endif

/** @}
 */
