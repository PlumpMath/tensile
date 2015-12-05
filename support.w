/*@<.@>=*/
/*
  Copyright (c) 2015  Artem V. Andreev
  This file is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3, or (at your option)
  any later version.
  
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
#ifndef SUPPORT_H
#define SUPPORT_H 1

#ifdef __cplusplus
extern "C"
{
#endif
  
#include <stddef.h>
#include <limits.h>
#include <inttypes.h>

/*@<Compiler attributes@>*/
/*@<Bit counting@>*/
/*@<Miscellanea@>*/

#ifdef __cplusplus
}
#endif
#endif
/*@ */
/*@<tests/support_ts.c@>=*/
#include "support.h"
#include "assertions.h"

TESTSUITE("Support utiltities");

/*@<Test cases@>*/

/*@ */
/*@<Compiler attributes@>=*/
/*@<Memory-related attributes@>*/
/*@<Side effect attributes@>*/
/*@<Sanity check attributes@>*/
/*@<Linking attributes@>*/
/*@<Usage hints@>*/

/*@ */
/*@<Memory-related attributes@>=*/
/* Indicates that the function returns a pointer to unaliased uninitalized memory */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define ATTR_MALLOC __attribute__((__malloc__))
#else
#define ATTR_MALLOC
#endif

/* 
 * Indicates that a function returns a pointer to memory, the size of
 * which is given in its |x|'th argument
 */  
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ATTR_ALLOC_SIZE(x) __attribute__((__alloc_size__(x)))
#else
#define ATTR_ALLOC_SIZE(x)
#endif

/* 
 * Indicates that a function returns a pointer to memory, which consists
 * of a number of elements given in its |x|'th argument, where each
 * element has the size given in the |y|'th argument
 */  
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ATTR_ALLOC_SIZE2(x,y) __attribute__((__alloc_size__(x,y)))
#else
#define ATTR_ALLOC_SIZE2(x,y)
#endif
  
/*@ */
/*@<Side effect attributes@>=*/
/* Indicates that the function has no side effects */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define ATTR_PURE __attribute__((__pure__))
#else
#define ATTR_PURE
#endif

/* Indicates that a function value only depends on its arguments, and
 * that the function produces no side effects,
 * cf. |ATTR_PURE|
 */  
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ATTR_CONST  __attribute__((__const__))
#else 
#define ATTR_CONST
#endif

/*@ */
/*@<Sanity check attributes@>=*/
/* Indicates that a vararg function shall have NULL at the end of varargs */  
#if __GNUC__ >= 4
#define ATTR_SENTINEL  __attribute__((__sentinel__))
#else
#define ATTR_SENTINEL
#endif

/* Indicates that a function is printf-like, and the format string is
 *  in the |_format_idx|'th argument
 */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ATTR_PRINTF(_format_idx, _arg_idx)                          \
    __attribute__((__format__ (__printf__, _format_idx, _arg_idx)))
#else 
#define ATTR_PRINTF(_format_idx, _arg_idx)
#endif

/* Indicates that a function is scanf-like, and the format string is
 *  in the |_format_idx|'th argument
 */  
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ATTR_SCANF(_format_idx, _arg_idx)                           \
    __attribute__((__format__ (__scanf__, _format_idx, _arg_idx)))
#else 
#define ATTR_SCANF(_format_idx, _arg_idx)
#endif

/* Indicates that a |_arg_idx| 'th argument to the function is 
 *  a printf/scanf/strftime format string that is transformed in some way
 *  and returned by the function
 */  
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ATTR_FORMAT(_arg_idx)                   \
    __attribute__((__format_arg__ (_arg_idx)))
#else 
#define ATTR_FORMAT(_arg_idx)
#endif

/* Indicates that no pointer arguments should ever be passed |NULL| */  
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ATTR_NONNULL __attribute__ ((__nonnull__))
#else
#define ATTR_NONNULL
#endif

/* Indicates that pointer arguments listed in |_args| are never |NULL| */ 
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ATTR_NONNULL_ARGS(_args) __attribute__ ((__nonnull__ _args))
#else
#define ATTR_NONNULL_ARGS(_args)
#endif

/* Indicates that the first argument is never |NULL| */
#define ATTR_NONNULL_1ST ATTR_NONNULL_ARGS((1))

/*@ */    
/*@<Return value attributes@>=*/

/* Indicates that a function returns a non-|NULL| pointer */ 
#if     (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define ATTR_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define ATTR_RETURNS_NONNULL 
#endif

/* Indicates that the function never returns */  
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ATTR_NORETURN __attribute__((__noreturn__))
#else 
#define ATTR_NORETURN
#endif

/* Requires a warning if a result value of the function is throw away */  
#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define ATTR_WARN_UNUSED_RESULT               \
  __attribute__((__warn_unused_result__))
#else
#define ATTR_WARN_UNUSED_RESULT
#endif

/*@ */
/*@<Linking attributes@>=*/
/* Marks the symbol as weak, that is a library symbol that can be
 * overriden in the application code 
 */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define ATTR_WEAK __attribute__((weak))
#else
#define ATTR_WEAK
#endif

/* Indicates ELF symbol visibility (default, hidden, internal or protected) */  
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ATTR_VISIBILITY(_scope) \
    __attribute__ ((__visibility__ (#_scope)))
#else
#define ATTR_VISIBILITY(_scope)
#endif

/*@ */
/*@<Usage hints@>=*/
/* Indicates that an object may remain unused */  
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ATTR_UNUSED __attribute__((__unused__))
#else
#define ATTR_UNUSED
#endif

/* Marks the symbol as deprecated */  
#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define ATTR_DEPRECATED __attribute__((__deprecated__))
#else
#define ATTR_DEPRECATED
#endif

/*@ */
/*@<Bit counting@>=*/

#if __GNUC__ >= 4
static inline unsigned count_leading_zeroes(size_t i)
{
    if (i == 0u)
        return sizeof(i) * CHAR_BIT;
    return (unsigned)__builtin_clzl(i);
}
#else
/* Number of leading zero bits in a 32-bit unsigned integer */
static inline unsigned count_leading_zeroes(size_t i)
{
    unsigned j;
  
    if (i == 0)
        return sizeof(i) * CHAR_BIT;
    for (j = sizeof(i) * CHAR_BIT - 1; j > 0; j--)
    {
        if ((i & (1ul << j)) != 0)
            return (unsigned)sizeof(i) * CHAR_BIT - 1 - j;
    }
    return sizeof(i) * CHAR_BIT - 1;
}
#endif

/*@ */
/*@<Test cases@>=*/
TESTCASE(count_leading_zeroes, "Count leading zeroes")
{
    TEST_FOREACH(clz,
                 size_t val;
                 unsigned expected,
                 {
                     TEST_LOG("%zx", clz->val);
                     ASSERT_EQ(bits,
                               count_leading_zeroes(clz->val),
                               clz->expected);
                 },
                 {0u, sizeof(size_t) * CHAR_BIT},            
                 {1u, sizeof(size_t) * CHAR_BIT - 1},
                 {0x12340u, sizeof(size_t) * CHAR_BIT - 17},
                 {UINT_MAX, (sizeof(size_t) - sizeof(unsigned)) * CHAR_BIT},
                 {SIZE_MAX, 0},
                 {SIZE_MAX >> 1, 1},
                 {SIZE_MAX >> 2, 2});
}

/*@ */
/*@<Miscellanea@>=*/
/* Make a prefix-qualified name */
#define _QNAME(_prefix, _name) _prefix##_##_name
#define QNAME(_prefix, _name) _QNAME(_prefix, _name)

#define DEFN_SCOPE_extern
#define DEFN_SCOPE_static static
#define DEFN_SCOPE(_declscope) MAKE_NAME(DEFN_SCOPE, _declscope)  

