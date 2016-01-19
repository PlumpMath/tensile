/****h Library/compiler.h
 * COPYRIGHT
 * (c) 2015-2016  Artem V. Andreev
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
 *
 * NAME
 * compiler.h --- Compiler support stuff.
 *
 * FUNCTION
 * Compilers offer various extensions over strict C standard, and
 * also implement various versions of ISO C (sometimes, only partially).
 * This file offers compatibility wrappers around some of those extensions.
 *
 * AUTHOR 
 * Artem V. Andreev <artem@AA5779.spb.edu>
 *****/
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

/****h compiler.h/Annotations
 * FUNCTION
 * Modern compilers, in particular GCC and Clang, support a rich set of
 * annotations to improve code quality. 
 * Unfortunately, various versions of those compilers implement various sets
 * of the attributes, hence we need to wrap them into macros, conditioned by
 * the compiler version
 *****/

/***** Annotations/returns
 * NAME
 * returns(x) --- return value annotation
 */
#define returns(_x) ANNOTATION_RETURNS_##_x

/* 
 * TAGS
 * - fresh_pointer: Indicates that the function returns 
 *   a pointer to unaliased uninitalized memory 
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define ANNOTATION_RETURNS_fresh_pointer __attribute__((__malloc__))
#else
#define ANNOTATION_RETURNS_fresh_pointer
#endif

/* 
 * - not_null: Indicates that a function returns a non-NULL pointer 
 */ 
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define ANNOTATION_RETURNS_not_null __attribute__((__returns_nonnull__))
#else
#define ANNOTATION_RETURNS_not_null
#endif

/* 
 * - important: Requires a warning if a result value 
 *   of the function is thrown away 
 */  
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define ANNOTATION_RETURNS_important            \
    __attribute__((__warn_unused_result__))
#else
#define ANNOTATION_RETURNS_important
#endif

/******/

/****** Annotations/arguments
 * NAME
 * arguments(x) --- argument list annotations
 */
#define arguments(_x) ANNOTATION_ARGUMENTS_##_x

/* 
 * TAGS
 * - sentinel: Indicates that a vararg function shall have NULL 
 *   at the end of varargs 
 */
#if __GNUC__ >= 4
#define ANNOTATION_ARGUMENTS_sentinel  __attribute__((__sentinel__))
#else
#define ANNOTATION_ARGUMENTS_sentinel
#endif

/* 
 * - not_null: Indicates that no pointer arguments should 
 *   ever be passed NULL 
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ANNOTATION_ARGUMENTS_not_null __attribute__ ((__nonnull__))
#else
#define ANNOTATION_ARGUMENTS_not_null
#endif

/******/


/****** Annotations/argument
 * NAME
 * argument(x, ...) --- specific argument annotations
 */
#define argument(_x, ...) ANNOTATION_ARGUMENT_##_x(__VA_ARGS__)

/* 
 * TAGS
 * - storage_size (_x): Indicates that a function returns 
 *   a pointer to memory, the size of which is given in its _x'th argument
 * - storage_size (_x, _y): Indicates that a function returns 
 *   a pointer to memory, which consists of a number of elements 
 *   given in its x'th argument, where each element has the size given in 
 *   the y'th argument
 */
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define ANNOTATION_ARGUMENT_storage_size_AT(...)    \
    __attribute__((__alloc_size__(__VA_ARGS__)))
#else
#define ANNOTATION_ARGUMENT_storage_size(...)
#endif

/* 
 * - printf (_x, _y): Indicates that a function is printf-like, 
 *   and the format string is in the _x'th argument, and
 *   the arguments start at _y
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ANNOTATION_ARGUMENT_printf(_x, _y)              \
    __attribute__((__format__ (__printf__, _x, _y)))
#else 
#define ANNOTATION_ARGUMENT_printf(_x, _y)
#endif

/* 
 * - scanf (_x, _y): Indicates that a function is scanf-like, 
 *   and the format string is in the _x'th argument, and
 *   the arguments start at _y
 */  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ANNOTATION_ARGUMENT_scanf(_x, _arg_y)       \
    __attribute__((__format__ (__scanf__, _x, _y)))
#else 
#define ANNOTATION_ARGUMENT_scanf(_x, _y)
#endif

/* 
 * - format_string (_x): Indicates that a _x'th argument to the function is 
 *  a printf/scanf/strftime format string that is transformed in some way
 *  and returned by the function
 */  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ANNONATION_ARGUMENT_format_string(_x) \
    __attribute__((__format_arg__ (_x)))
#else 
#define ANNOTATION_ARGUMENT_format_string(_x)
#endif


/* 
 * - not_null (_args...): Indicates that pointer arguments listed in 
 *   _args are never NULL
 */ 
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)
#define ANNOTATION_ARGUMENT_not_null(...) \
    __attribute__ ((__nonnull__ (__VA_ARGS__)))
#else
#define ANNOTATION_ARGUMENT_not_null(...)
#endif

/*****/

/****** Annotations/C99-11
 * FUNCTION
 * Compilers vary in their support for C99 and C11 features,
 * and the level of support may be altered by command-line switches
 *
 * TAGS
 * - restrict: Restricted pointers are supported by GCC even in non-C99 mode
 *   but with an alternative keyword __restrict__
 */
#if __STDC_VERSION__ < 199901L
#if __GNUC__
#define restrict __restrict__
#else
#define restrict
#endif
#endif

/*
 * - static_assert: Defined in <assert.h> by C11-compliant systems. 
 *   If not defined, revert to an old trick of using negative array size
 */
#if !defined(static_assert)
#define static_assert(_cond, _msg) ((void)sizeof(char[(_cond) ? 1 : -1]))
#endif

/*
 * - at_least(_x): A C99 feature to specify a mininum required number of 
 *   array elements when an array is passed as parameter. Supported by GCC
 *   in its default mode if no strict ISO compliance is requested.
 *   Since C99 reuses `static' for this purpose, we need to conditionally
 *   define our own macro
 */
#if __STDC_VERSION__ >= 199901L || (defined(__GNUC__) && !__STRICT_ANSI__) 
#define at_least(_x) static _x
#else
#define at_least(_x) _x
#endif

/* 
 * - noreturn: For C11 systems, it is defined in a new header
 *   <stdnoreturn.h>. For GCC in non-C11 mode define it via noreturn
 *   attribute
 */
#if __STDC_VERSION__ >= 201112L
#include <stdnoreturn.h>
#elif     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define noreturn __attribute__((__noreturn__))
#else 
#define noreturn
#endif

/*****/


/****** Annotations/global_state
 * NAME
 * global_state(_x) --- Global state affected by the function
 */
#define global_state(_x) ANNOTATION_GLOBAL_STATE_##_x

/*
 * TAGS
 * - modify: Global state may be modified. This is the default, so
 *   it is expanded to nothing and is provided only for completeness
 */
#define ANNOTATION_GLOBAL_STATE_modify

/*
 * - read: Global state may be read but not modified. In particular,
 *   that means a function cannot produce any observable side effects
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define ANNOTATION_GLOBAL_STATE_read __attribute__((__pure__))
#else
#define ANNOTATION_GLOBAL_STATE_read
#endif

/*
 * - none: No global state is accessed by the function, and so the
 *   function is genuinely function, its result depending solely on its
 *   arguments
 */  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ANNOTATION_GLOBAL_STATE_none  __attribute__((__const__))
#else 
#define ANNOTATION_GLOBAL_STATE_none
#endif

/*****/


/****** Annotations/linkage
 * NAME
 * linkage(_x) --- Additional linkage modes
 */
#define linkage(_x) ANNOTATION_LINKAGE_##_x

/*
 * TAGS
 * - weak: Weak symbol (a library symbol that may be overriden
 *   by the application
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define ANNOTATION_LINKAGE_weak __attribute__((__weak__))
#else
#define ANNOTATION_LINKAGE_weak
#endif

/*
 * - export: A symbol is exported from a shared library
 * - import: A symbol is imported from a shared library
 *   For POSIX systems these two modes are void.
 */
#if __WIN32
#define ANNOTATION_LINKAGE_export __declspec(dllexport)
#define ANNOTATION_LINKAGE_import __declspec(dllimport)
#else
#define ANNOTATION_LINKAGE_export
#define ANNOTATION_LINKAGE_import
#endif

/*
 * - local: A symbol is NOT exported from a shared library
 *   (but unlike static symbols, it is visible to all compilation units
 *   comprising the library itself)
 * - internal: Like `local', but the symbol cannot be accessed by other
 *   modules even indirectly (e.g. through a function pointer)
 */
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 3)) && __ELF__
#define ANNOTATION_LINKAGE_local                \
    __attribute__ ((__visibility__ ("hidden")))
#define ANNOTATION_LINKAGE_internal                 \
    __attribute__ ((__visibility__ ("internal")))
#else
#define ANNOTATION_LINKAGE_local
#define ANNOTATION_LINKAGE_internal
#endif


/* 
 * - deprecated: The symbol should not be used and triggers a warning 
 */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define ANNOTATION_LINKAGE_deprecated __attribute__((__deprecated__))
#else
#define ANNOTATION_LINKAGE_deprecated
#endif

/*****/


/****** Annotations/unused
 * NAME
 * unused -- Marks a symbol as explicitly unused
 */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define unused __attribute__((__unused__))
#else
#define unused
#endif

/*****/

/****f* compiler.h/QNAME
 * NAME
 * QNAME(_prefix, _name) --- Make a qualifed name
 *
 * RESULT
 * Produces a symbol composed of _prefix and _name
 * separated by an underscore
 */
#define _QNAME(_prefix, _name) _prefix##_##_name
#define QNAME(_prefix, _name) _QNAME(_prefix, _name)

#ifndef PROBE
#define PROBE(_name) ((void)0)
#endif

#ifdef __cplusplus
}
#endif
#endif /* COMPILER_H */
