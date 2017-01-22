/**********************************************************************
 * Copyright (c) 2017 Artem V. Andreev
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**********************************************************************/
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

/** @name Annotations
 * Modern compilers, in particular GCC and Clang, support a rich set of
 * annotations to improve code quality.
 * Unfortunately, various versions of those compilers implement various sets
 * of the attributes, hence we need to wrap them into macros, conditioned by
 * the compiler version
 */

#define annotation(_kind, ...) ANNOTATION_##_kind(__VA_ARGS__)

/**@{*/

/**
 * Return value annotation
 */
#define ANNOTATION_returns(_x) ANNOTATION_returns__##_x

/**
 * Indicates that the function returns
 * a pointer to unaliased uninitalized memory
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define ANNOTATION_returns__fresh_pointer __attribute__((__malloc__))
#else
#define ANNOTATION_returns__fresh_pointer
#endif

/**
 * Indicates that a function returns a non-NULL pointer
 */
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define ANNOTATION_returns__not_null __attribute__((__returns_nonnull__))
#else
#define ANNOTATION_returns__not_null
#endif

/**
 * Requires a warning if a result value
 * of the function is thrown away
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define ANNOTATION_returns__important            \
    __attribute__((__warn_unused_result__))
#else
#define ANNOTATION_returns__important
#endif

/**
 * Argument list annotations
 */
#define ANNOTATION_arguments(_x) ANNOTATION_arguments__##_x

/**
 * Indicates that a vararg function shall have NULL
 * at the end of varargs
 */
#if __GNUC__ >= 4
#define ANNOTATION_arguments__sentinel  __attribute__((__sentinel__))
#else
#define ANNOTATION_arguments__sentinel
#endif

/**
 * Indicates that no pointer arguments should
 * ever be passed NULL
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ANNOTATION_arguments__not_null __attribute__ ((__nonnull__))
#else
#define ANNOTATION_arguments__not_null
#endif

/**
 * Specific argument annotations
 */
#define ANNOTATION_argument(_x, ...) ANNOTATION_argument__##_x(__VA_ARGS__)

/**
 * Indicates that a function returns
 * a pointer to memory, the size of which is given in its _x'th argument.
 * Indicates that a function returns
 * a pointer to memory, which consists of a number of elements
 * given in its x'th argument, where each element has the size given in
 * the y'th argument
 */
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ANNOTATION_argument__storage_size(...)      \
    __attribute__((__alloc_size__(__VA_ARGS__)))
#else
#define ANNOTATION_argument__storage_size(...)
#endif

/**
 * Indicates that a function returns
 * a pointer to memory, the alignment of which is given in its _x'th argument.
 */
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define ANNOTATION_argument__storage_align(...)     \
    __attribute__((__alloc_align__(__VA_ARGS__)))
#else
#define ANNOTATION_argument__storage_align(...)
#endif

/**
 * Indicates that a function is printf-like,
 * and the format string is in the _x'th argument, and
 * the arguments start at _y
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ANNOTATION_argument__printf(_x, _y)             \
    __attribute__((__format__ (__printf__, _x, _y)))
#else
#define ANNOTATION_argument__printf(_x, _y)
#endif

/**
 * Indicates that a function is scanf-like,
 * and the format string is in the _x'th argument, and
 * the arguments start at _y
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ANNOTATION_argument__scanf(_x, _arg_y)      \
    __attribute__((__format__ (__scanf__, _x, _y)))
#else
#define ANNOTATION_argument__scanf(_x, _y)
#endif

/**
 * Indicates that a _x'th argument to the function is
 * a printf/scanf/strftime format string that is transformed in some way
 * and returned by the function
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ANNONATION_argument__format_string(_x)  \
    __attribute__((__format_arg__ (_x)))
#else
#define ANNOTATION_argument__format_string(_x)
#endif


/**
 * Indicates that pointer arguments listed in
 * `_args` are never `NULL`
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ANNOTATION_argument__not_null(...)      \
    __attribute__ ((__nonnull__ (__VA_ARGS__)))
#else
#define ANNOTATION_argument__not_null(...)
#endif

/**
 * Global state affected by the function
 */
#define ANNOTATION_global_state(_x) ANNOTATION_global_state__##_x

/**
 * Global state may be modified. This is the default, so
 * it is expanded to nothing and is provided only for completeness
 */
#define ANNOTATION_global_state__modify

/**
 * Global state may be read but not modified. In particular,
 * that means a function cannot produce any observable side effects
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define ANNOTATION_global_state__read __attribute__((__pure__))
#else
#define ANNOTATION_global_state__read
#endif

/**
 * No global state is accessed by the function, and so the
 * function is genuinely function, its result depending solely on its
 * arguments
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ANNOTATION_global_state__none  __attribute__((__const__))
#else
#define ANNOTATION_global_state__none
#endif

/**
 * Additional linkage modes
 */
#define linkage(_x) LINKAGE__##_x

/**
 * Weak symbol (a library symbol that may be overriden
 * by the application
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define LINKAGE__weak __attribute__((__weak__))
#else
#define LINKAGE__weak
#endif

/** @def ANNOTATION_LINKAGE_export
 * A symbol is exported from a shared library
 */
/** @def ANNOTATION_LINKAGE_import
 * A symbol is imported from a shared library
 * For POSIX systems these two modes are void.
 */
#if __WIN32
#define LINKAGE__export __declspec(dllexport)
#define LINKAGE__import __declspec(dllimport)
#else
#define LINKAGE__export
#define LINKAGE__import
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
#define LINKAGE__local                \
    __attribute__ ((__visibility__ ("hidden")))
#define LINKAGE__internal                 \
    __attribute__ ((__visibility__ ("internal")))
#else
#define LINKAGE__local
#define LINKAGE__internal
#endif


/**
 * The symbol should not be used and triggers a warning
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define LINKAGE__deprecated __attribute__((__deprecated__))
#else
#define LINKAGE__deprecated
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
