/*@* Copyright (c) 2015  Artem V. Andreev
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
/*@* Compiler support stuff.

  Compilers offer various extensions over strict C standard, and
  also implement various versions of ISO C (sometimes, only partially).
  This file offers compatibility wrappers around some of those extensions.
 */
/*@<*@>=*/
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
/*@* Annotations.

  Modern compilers, in particular GCC and Clang, support a rich set of
  annotations to improve code quality. 
  Unfortunately, various versions of those compilers implement various sets
  of the attributes, hence we need to wrap them into macros, conditioned by
  the compiler version
*/
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
#define MALLOC_LIKE __attribute__((__malloc__))
#else
#define MALLOC_LIKE
#endif

/* 
 * Indicates that a function returns a pointer to memory, the size of
 * which is given in its |x|'th argument
 */  
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ALLOC_SIZE_AT(_x) __attribute__((__alloc_size__(_x)))
#else
#define ALLOC_SIZE_AT(_x)
#endif

/* 
 * Indicates that a function returns a pointer to memory, which consists
 * of a number of elements given in its |x|'th argument, where each
 * element has the size given in the |y|'th argument
 */  
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ALLOC_SIZE_AT2(_x, _y) __attribute__((__alloc_size__(_x, _y)))
#else
#define ALLOC_SIZE_AT2(_x, _y)
#endif


#if __STDC_VERSION__ >= 199901L
#define RESTRICT restrict
#else
#define RESTRICT
#endif

#if __STDC_VERSION__ >= 199901L || (defined(__GNUC__) && !__STRICT_ANSI__) 
#define AT_LEAST(_x) static _x
#else
#define AT_LEAST(_x) _x
#endif

/*@ */
/*@<Side effect attributes@>=*/
/* Indicates that the function has no side effects */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define PURE __attribute__((__pure__))
#else
#define PURE
#endif

/* Indicates that a function value only depends on its arguments, and
 * that the function produces no side effects,
 * cf. |PURE|
 */  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define CONSTANT  __attribute__((__const__))
#else 
#define CONSTANT
#endif

/*@ */
/*@<Sanity check attributes@>=*/
/* Indicates that a vararg function shall have NULL at the end of varargs */  
#if __GNUC__ >= 4
#define HAS_SENTINEL  __attribute__((__sentinel__))
#else
#define HAS_SENTINEL
#endif

/* Indicates that a function is printf-like, and the format string is
 *  in the |_format_idx|'th argument
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define PRINTF_LIKE(_format_idx, _arg_idx)                            \
    __attribute__((__format__ (__printf__, _format_idx, _arg_idx)))
#else 
#define PRINTF_LIKE(_format_idx, _arg_idx)
#endif

/* Indicates that a function is scanf-like, and the format string is
 *  in the |_format_idx|'th argument
 */  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define SCANF_LIKE(_format_idx, _arg_idx)                           \
    __attribute__((__format__ (__scanf__, _format_idx, _arg_idx)))
#else 
#define SCANF_LIKE(_format_idx, _arg_idx)
#endif

/* Indicates that a |_arg_idx| 'th argument to the function is 
 *  a printf/scanf/strftime format string that is transformed in some way
 *  and returned by the function
 */  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define FORMAT_AT(_arg_idx)                     \
    __attribute__((__format_arg__ (_arg_idx)))
#else 
#define FORMAT_AT(_arg_idx)
#endif

/* Indicates that no pointer arguments should ever be passed |NULL| */  
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define NONNULL_ALL __attribute__ ((__nonnull__))
#else
#define NONNULL_ALL
#endif

/* Indicates that pointer arguments listed in |_args| are never |NULL| */ 
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define NONNULL(...) __attribute__ ((__nonnull__ (__VA_ARGS___)))
#else
#define NONNULL(...)
#endif

/*@ */    
/*@<Return value attributes@>=*/

/* Indicates that a function returns a non-|NULL| pointer */ 
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define RETURNS_NONNULL 
#endif

/* Indicates that the function never returns */
#if __STDC_VERSION__ >= 201112L
#include <stdnoreturn.h>
#define NORETURN noreturn
#elif     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define NORETURN __attribute__((__noreturn__))
#else 
#define NORETURN
#endif

/* Requires a warning if a result value of the function is throw away */  
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define WARN_UNUSED_RESULT               \
  __attribute__((__warn_unused_result__))
#else
#define WARN_UNUSED_RESULT
#endif

/*@ */
/*@<Linking attributes@>=*/
/* Marks the symbol as weak, that is a library symbol that can be
 * overriden in the application code 
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

/* Indicates ELF symbol visibility (default, hidden, internal or protected) */  
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define NOEXPORT \
    __attribute__ ((__visibility__ ("hidden")))
#define INTERNAL \
    __attribute__ ((__visibility__ ("internal")))
#else
#define NOEXPORT
#define INTERNAL
#endif

/*@ */
/*@<Usage hints@>=*/
/* Indicates that an object may remain unused */  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif

/* Marks the symbol as deprecated */  
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define DEPRECATED __attribute__((__deprecated__))
#else
#define DEPRECATED
#endif

/*@ 
  Number of leading zero bits in an unsigned integer
*/
/*@<Bit counting@>=*/

#if __GNUC__ >= 4
static inline unsigned count_leading_zeroes(size_t i)
{
    if (i == 0u)
        return sizeof(i) * CHAR_BIT;
    return (unsigned)__builtin_clzl(i);
}
#else
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
TESTCASE(count_leading_zeroes, "Count leading zeroes",,,
         (size_t val, unsigned expected),
         T(0u, sizeof(size_t) * CHAR_BIT),
         T(1u, sizeof(size_t) * CHAR_BIT - 1),
         T(0x12340u, sizeof(size_t) * CHAR_BIT - 17),
         T(UINT_MAX, (sizeof(size_t) - sizeof(unsigned)) * CHAR_BIT),
         T(SIZE_MAX, 0),
         T(SIZE_MAX >> 1, 1),
         T(SIZE_MAX >> 2, 2))
{
    TEST_LOG("%zx", val);
    ASSERT_EQ(bits,
              count_leading_zeroes(val),
              expected);
}

/*@ 
  Make a prefix-qualified name 
 */
/*@<Miscellanea@>=*/
#define _QNAME(_prefix, _name) _prefix##_##_name
#define QNAME(_prefix, _name) _QNAME(_prefix, _name)

#define DEFN_SCOPE_extern
#define DEFN_SCOPE_static static
#define DEFN_SCOPE(_declscope) _QNAME(DEFN_SCOPE, _declscope)  

/*@ And at last some testing trivia...
 */
/*@<tests/support_ts.c@>=*/
#include "support.h"
#include "assertions.h"

TESTSUITE("Support utiltities");

/*@<Test cases@>*/
