/*
 * Copyright (c) 2015-2016  Artem V. Andreev
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/** @file
 * @brief Compiler support
 *
 * Compilers offer various extensions over strict C standard, and
 * also implement various versions of ISO C (sometimes, only partially).
 * This file offers compatibility wrappers around some of those extensions.
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef COMPILER_H
#define COMPILER_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <inttypes.h>
#include <assert.h>


/**
 * Indicates that the function returns
 * a pointer to unaliased uninitalized memory
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define hint_returns_fresh_pointer __attribute__((__malloc__))
#else
#define hint_returns_fresh_pointer
#endif

/**
 * Indicates that a function returns a non-NULL pointer
 */
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define hint_returns_not_null __attribute__((__returns_nonnull__))
#else
#define hint_returns_not_null
#endif

/**
 * Requires a warning if a result value
 * of the function is thrown away
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define warn_unused_result     __attribute__((__warn_unused_result__))
#else
#define warn_unused_result
#endif

/**
 * Indicates that a vararg function shall have NULL
 * at the end of varargs
 */
#if __GNUC__ >= 4
#define warn_last_vararg_not_null  __attribute__((__sentinel__))
#else
#define warn_last_vararg_not_null
#endif

/**
 * Indicates that no pointer arguments should
 * ever be passed NULL
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define warn_any_null_arg  __attribute__ ((__nonnull__))
#else
#define warn_any_null_arg
#endif


/**
 * Indicates that a function returns
 * a pointer to memory, the size of which is given in its _x'th argument.
 * Indicates that a function returns
 * a pointer to memory, which consists of a number of elements
 * given in its x'th argument, where each element has the size given in
 * the y'th argument
 */
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define hint_malloc_like(_arg)                  \
    hint_returns_fresh_pointer                  \
    __attribute__((__alloc_size__(_arg)))
#define hint_calloc_like(_arg1, _arg2)              \
    hint_returns_fresh_pointer                      \
    __attribute__((__alloc_size__(_arg1, _arg2)))
#else
#define hint_malloc_like(_arg) hint_returns_fresh_pointer
#define hint_calloc_like(_arg1, _arg2) hint_returns_fresh_pointer
#endif

/**
 * Indicates that a function returns
 * a pointer to memory, the alignment of which is given in its _x'th argument.
 */
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define hint_memalign_like(_arg1, _arg2)                  \
    __attribute__((__alloc_align__(_arg1)))               \
    hint_malloc_like(_arg2)
#else
#define hint_memalign_like(_arg1, _arg2) hint_malloc_like(_arg2)
#endif

/**
 * Indicates that a function is printf-like,
 * and the format string is in the _x'th argument, and
 * the arguments start at _y
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define hint_printf_like(_x, _y)                        \
    __attribute__((__format__ (__printf__, _x, _y)))
#else
#define hint_printf_like(_x, _y)
#endif

/**
 * Indicates that a function is scanf-like,
 * and the format string is in the _x'th argument, and
 * the arguments start at _y
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define hint_scanf_like(_x, _arg_y)                      \
    __attribute__((__format__ (__scanf__, _x, _y)))
#else
#define hint_scanf_like(_x, _y)
#endif

/**
 * Indicates that a _x'th argument to the function is
 * a printf/scanf/strftime format string that is transformed in some way
 * and returned by the function
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define hint_arg_format_string(_x)  \
    __attribute__((__format_arg__ (_x)))
#else
#define hint_arg_format_string(_x)
#endif


/**
 * Indicates that pointer arguments listed in
 * `_args` are never `NULL`
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define warn_null_args(...)                     \
    __attribute__ ((__nonnull__ (__VA_ARGS__)))
#else
#define warn_null_args(...)
#endif


/**
 * Global state may be read but not modified. In particular,
 * that means a function cannot produce any observable side effects
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define hint_no_side_effects __attribute__((__pure__))
#else
#define hint_no_side_effects
#endif

/**
 * No global state is accessed by the function, and so the
 * function is genuinely function, its result depending solely on its
 * arguments
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define hint_no_shared_state  __attribute__((__const__))
#else
#define hint_no_shared_state
#endif


/**
 * Weak symbol (a library symbol that may be overriden
 * by the application
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define weak_linkage __attribute__((__weak__))
#else
#define weak_linkage
#endif

/** @def ANNOTATION_LINKAGE_export
 * A symbol is exported from a shared library
 */
/** @def ANNOTATION_LINKAGE_import
 * A symbol is imported from a shared library
 * For POSIX systems these two modes are void.
 */
#if __WIN32
#define dll_export_linkage __declspec(dllexport)
#define dll_import_linkage __declspec(dllimport)
#else
#define dll_export_linkage
#define dll_import_linkage
#endif

/** @def ANNOTATION_LINKAGE_local
 * symbol is **not** exported from a shared library
 * (but unlike static symbols, it is visible to all compilation units
 * comprising the library itself)
 */
/**  @def ANNOTATION_LINKAGE_internal
 * Like `local`, but the symbol cannot be accessed by other
 * modules even indirectly (e.g. through a function pointer)
 */
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)) && __ELF__
#define local_linkage                           \
    __attribute__ ((__visibility__ ("hidden")))
#define internal_linkage                            \
    __attribute__ ((__visibility__ ("internal")))
#else
#define local_linkage
#define internal_linkage
#endif


/**
 * The symbol should not be used and triggers a warning
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define deprecated __attribute__((__deprecated__))
#else
#define deprecated
#endif

/**
 * Marks a symbol as explicitly unused
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define unused __attribute__((__unused__))
#else
#define unused
#endif

/**@}*/

/** @name C99/C11 annotations
 * Compilers vary in their support for C99 and C11 features,
 * and the level of support may be altered by command-line switches
 */
/**@{*/

/**
 * Restricted pointers are supported by GCC even in non-C99 mode
 *  but with an alternative keyword `__restrict__`
 */
#if __STDC_VERSION__ < 199901L
#if __GNUC__
#define restrict __restrict__
#else
#define restrict
#endif
#endif

/**
 * Defined in <assert.h> by C11-compliant systems.
 * If not defined, revert to an old trick of using negative array size
 */
#if !defined(static_assert)
#define static_assert(_cond, _msg) ((void)sizeof(char[(_cond) ? 1 : -1]))
#endif

/**
 * A C99 feature to specify a mininum required number of
 * array elements when an array is passed as parameter. Supported by GCC
 * in its default mode if no strict ISO compliance is requested.
 * Since C99 reuses `static' for this purpose, we need to conditionally
 * define our own macro
 */
#if __STDC_VERSION__ >= 199901L || (defined(__GNUC__) && !__STRICT_ANSI__)
#define at_least(_x) static _x
#else
#define at_least(_x) _x
#endif

/**
 * C99 allows arrays to be declared in function declarations
 * with non-constant dimensions, depending on previous arguments.
 * It does not affect the produced code and no compile- or run-time
 * checks are usually performed, but the code intent is more evident 
 * this way
 */
#if (__STDC_VERSION__ >= 199901L && !__STDC_NO_VLA__) ||    \
    (defined(__GNUC__) && !__STRICT_ANSI__)
#define var_size(_x) _x
#else
#define var_size(_x)
#endif
    
/**
 * For C11 systems, it is defined in a new header
 * <stdnoreturn.h>. For GCC in non-C11 mode define it via noreturn
 * attribute
 */
#if __STDC_VERSION__ >= 201112L
#include <stdnoreturn.h>
#elif     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define noreturn __attribute__((__noreturn__))
#else
#define noreturn
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define constructor static __attribute__((__constructor__))
#define destructor static __attribute__((__destructor__))
#else
#define constructor constructors_are_not_supported
#define destructor constructors_are_not_supported
#endif

/**@}*/

/**
 * Make an unexpanded qualifed name
 *
 * Produces a symbol composed of _prefix and _name
 * separated by an underscore, where _prefix and _name
 * are not macro-expanded
 */
#define _QNAME(_prefix, _name) _prefix##_##_name

/**
 * Make a qualifed name
 *
 * Produces a symbol composed of _prefix and _name
 * separated by an underscore
 */
#define QNAME(_prefix, _name) _QNAME(_prefix, _name)

#ifdef __cplusplus
}
#endif
#endif /* COMPILER_H */
