/*
 * Copyright (c) 2015  Artem V. Andreev
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
/** @file
 * @brief supporting macros
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef SUPPORT_H
#define SUPPORT_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <limits.h>

/** @cond DEV */

/* Info on which gcc versions support which attributes is mostly borrowed from
 * glib/gmacro.h header
 */

#if    __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define ATTR_PURE __attribute__((__pure__))
#define ATTR_MALLOC __attribute__((__malloc__))
#else
/** Indicates that the function has no side effects */
#define ATTR_PURE
/** Indicates that the function returns a pointer to unaliased uninitalized memory */
#define ATTR_MALLOC
#endif

#if     __GNUC__ >= 4
#define ATTR_SENTINEL  __attribute__((__sentinel__))
#else
/** Indicates that a vararg function shall have NULL at the end of varargs */
#define ATTR_SENTINEL
#endif

#if     (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ATTR_ALLOC_SIZE(x) __attribute__((__alloc_size__(x)))
#define ATTR_ALLOC_SIZE2(x,y) __attribute__((__alloc_size__(x,y)))
#else
/** 
 * Indicates that a function returns a pointer to memory, the size of
 * which is given in its @p x 'th argument
 */
#define ATTR_ALLOC_SIZE(x)

/** 
 * Indicates that a function returns a pointer to memory, which consists
 * of a number of elements given in its @p x 'th argument, where each
 * element has the size given in the @p y 'th argument
 */
#define ATTR_ALLOC_SIZE2(x,y)
#endif

#if     (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define ATTR_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
/** Indicates that a function returns a non-NULL pointer */
#define ATTR_RETURNS_NONNULL 
#endif

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define ATTR_WEAK __attribute__((weak)
#else
/** Marks the symbol as weak, that is a library symbol that can be
 * overriden in the application code 
 */
#define ATTR_WEAK
#endif

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ATTR_PRINTF(_format_idx, _arg_idx)                          \
    __attribute__((__format__ (__printf__, _format_idx, _arg_idx)))
#define ATTR_SCANF(_format_idx, _arg_idx)                           \
    __attribute__((__format__ (__scanf__, _format_idx, _arg_idx)))
#define ATTR_FORMAT(_arg_idx)                   \
    __attribute__((__format_arg__ (_arg_idx)))
#define ATTR_NORETURN __attribute__((__noreturn__))
#define ATTR_CONST  __attribute__((__const__))
#define ATTR_UNUSED __attribute__((__unused__))
#else   /* !__GNUC__ */
/** Indicates that a function is printf-like, and the format string is
 *  in the @p _format_idx 'th argument
 */
#define ATTR_PRINTF(_format_idx, _arg_idx)
/** Indicates that a function is scanf-like, and the format string is
 *  in the @p _format_idx 'th argument
 */
#define ATTR_SCANF(_format_idx, _arg_idx)
/** Indicates that a @p _arg_idx 'th argument to the function is 
 *  a printf/scanf/strftime format string that is transformed in some way
 *  and returned by the function
 */
#define ATTR_FORMAT(_arg_idx)
/** Indicates that the function never returns */
#define ATTR_NORETURN
/** Indicates that a function value only depends on its arguments, and
 * that the function produces no side effects
 * @sa ATTR_PURE
 */
#define ATTR_CONST
/** Indicates that an object may remain unused */
#define ATTR_UNUSED
#endif  /* !__GNUC__ */

#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define ATTR_DEPRECATED __attribute__((__deprecated__))
#else
/** Marks the symbol as deprecated */
#define ATTR_DEPRECATED
#endif /* __GNUC__ */

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ATTR_VISIBILITY(_scope) \
    __attribute__ ((__visibility__ (#_scope)))
#define ATTR_NONNULL __attribute__ ((__nonnull__))
#define ATTR_NONNULL_ARGS(_args) __attribute__ ((__nonnull__ _args))
#else
/** Indicates ELF symbol visibility (default, hidden, internal or protected) */
#define ATTR_VISIBILITY(_scope)
/** Indicates that no pointer arguments should ever be passed NULL */
#define ATTR_NONNULL
/** Indicates that pointer arguments listed in @p _args are never NULL */
#define ATTR_NONNULL_ARGS(_args)
#endif

/** Indicates that the first argument is never NULL */
#define ATTR_NONNULL_1ST ATTR_NONNULL_ARGS((1))

#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define ATTR_WARN_UNUSED_RESULT               \
  __attribute__((__warn_unused_result__))
#else
/** Requires a warning if a result value of the function is throw away */
#define ATTR_WARN_UNUSED_RESULT
#endif /* __GNUC__ */

/** @endcond */

#if __GNUC__ >= 4
static inline unsigned count_leading_zeroes(unsigned i)
{
    if (i == 0u)
        return (unsigned)sizeof(i) * CHAR_BIT;
    return (unsigned)__builtin_clz(i);
}
#else
/**
 * @return Number of leading zero bits in a 32-bit unsigned integer
 * @test
 *  Verify that the number of leading zeroes is correct
 * `ASSERT_UINT_EQ(count_leading_zeroes(i), expected);`
 *  `unsigned` @p i | `unsigned` @p expected
 *  ----------------|------------------------
 *  `0u`            | `sizeof(unsigned) * CHAR_BIT`
 *  `1u`            | `sizeof(unsigned) * CHAR_BIT - 1`
 *  `0x12340u`      | `sizeof(unsigned) * CHAR_BIT - 17`
 *  `UINT_MAX`      | `0`
 *  `UINT_MAX >> 1` | `1`
 *  `UINT_MAX >> 2` | `2`
 *
 */
static inline unsigned count_leading_zeroes(unsigned i)
{
    unsigned j;
  
    if (i == 0)
        return (unsigned)sizeof(i) * CHAR_BIT;
    for (j = sizeof(i) * CHAR_BIT - 1; j > 0; j--)
    {
        if (i & (1u << j))
            return (unsigned)sizeof(i) * CHAR_BIT - 1 - j;
    }
    return (unsigned)sizeof(i) * CHAR_BIT - 1;
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SUPPORT_H */
