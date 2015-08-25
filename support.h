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
/**
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

/* Info on which gcc versions support which attributes is mostly borrowed from
 * glib/gmacro.h header
 */

#if    __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define ATTR_PURE __attribute__((__pure__))
#define ATTR_MALLOC __attribute__((__malloc__))
#else
#define ATTR_PURE
#define ATTR_MALLOC
#endif

#if     __GNUC__ >= 4
#define ATTR_SENTINEL  __attribute__((__sentinel__))
#else
#define ATTR_SENTINEL
#endif

#if     (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ATTR_ALLOC_SIZE(x) __attribute__((__alloc_size__(x)))
#define ATTR_ALLOC_SIZE2(x,y) __attribute__((__alloc_size__(x,y)))
#else
#define ATTR_ALLOC_SIZE(x)
#define ATTR_ALLOC_SIZE2(x,y)
#endif

#if     (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define ATTR_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define ATTR_RETURNS_NONNULL 
#endif

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define ATTR_WEAK __attribute__((weak)
#else
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
#define ATTR_PRINTF(_format_idx, _arg_idx)
#define ATTR_SCANF(_format_idx, _arg_idx)
#define ATTR_FORMAT(_arg_idx)
#define ATTR_NORETURN
#define ATTR_CONST
#define ATTR_UNUSED
#endif  /* !__GNUC__ */

#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define ATTR_DEPRECATED __attribute__((__deprecated__))
#else
#define ATTR_DEPRECATED
#endif /* __GNUC__ */

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ATTR_VISIBILITY(_scope) \
    __attribute__ ((__visibility__ (#_scope)))
#define ATTR_NONNULL __attribute__ ((__nonnull__))
#define ATTR_NONNULL_ARGS(_args) __attribute__ ((__nonnull__ _args))
#else
#define ATTR_VISIBILITY(_scope)
#define ATTR_NONNULL
#define ATTR_NONNULL_ARGS(_args)
#endif

#define ATTR_NONNULL_1ST ATTR_NONNULL_ARGS((1))

#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define ATTR_WARN_UNUSED_RESULT               \
  __attribute__((__warn_unused_result__))
#else
#define ATTR_WARN_UNUSED_RESULT
#endif /* __GNUC__ */


#if __GNUC__ >= 4
static inline unsigned count_leading_zeroes(unsigned i)
{
  if (i == 0u)
    return 0u;
  return (unsigned)__builtin_clz(i);
}
#else
static inline unsigned count_leading_zeroes(unsigned i)
{
  unsigned j;
  
  if (i == 0)
    return 0;
  for (j = sizeof(i) * CHAR_BIT - 1; j > 0; j--)
  {
    if (i & (1u << j))
      return sizeof(i) * CHAR_BIT - 1 - j;
  }
  return sizeof(i) * CHAR_BIT - 1;
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SUPPORT_H */
