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
 * @brief fast object allocation macros
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 * @test Background:
 * @code
 * #define IMPLEMENT_ALLOCATOR 1
 * @endcode
 */
#ifndef ALLOCATOR_H
#define ALLOCATOR_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <assert.h>
#include <limits.h>
#include "support.h"

/**
 * Generates declarations for freelist-based typed memory allocations:
 * - `_type *new_<_type>(_args)`
 * - `void free_<_type>(void)`
 * - `_type *copy_<_type>(const _type *obj)`
 *
 * @test Background:
 * @code
 * typedef struct simple_type {
 *    void *ptr;
 *    unsigned tag;
 * } simple_type;
 *
 * DECLARE_TYPE_ALLOCATOR(simple_type, (unsigned tag));
 *
 * DEFINE_TYPE_ALLOCATOR(simple_type, (unsigned tag), obj,
 * { obj->tag = tag; },
 * { NEW(obj)->tag = OLD(obj)->tag + 1; },
 * { obj->tag = 0xdeadbeef; });
 * @endcode
 * @test Allocate
 * - Given a fresh object `simple_type *st = new_simple_type(0x12345);`
 * - Then it is allocated `ASSERT_PTR_NEQ(st, NULL);`
 * - And it is initialized `ASSERT_UINT_EQ(st->tag, 0x12345);`
 * - Clean up `free_simple_type(st);`
 * @test Allocate and free
 * - Given an allocated object `simple_type *st = new_simple_type(0x12345);`
 * - Then it is destroyed `free_simple_type(st);`
 * - And the free list is in proper state `ASSERT_PTR_EQ(freelist_simple_type, st);`
 * - And the object is finalized `ASSERT_UINT_EQ(st->tag, 0xdeadbeef);`
 * @test Allocate, free and reallocate
 * - Given an allocated object
 * @code
 * simple_type *st = new_simple_type(0x12345);
 * simple_type *st1;
 * @endcode
 * - When it is destroyed `free_simple_type(st);`
 * - And when another object is allocated `st1 = new_simple_type(0x54321);`
 * - Then the memory is reused `ASSERT_PTR_EQ(st, st1);`
 * - And the free list is in proper state `ASSERT_PTR_EQ(freelist_simple_type, NULL);`
 * - And the new object is initialized `ASSERT_UINT_EQ(st1->tag, 0x54321);`
 * - Clean up `free_simple_type(st1);`
 * @test Allocate, free, reallocate and allocate another
 * - Given an allocated object
 * @code
 * simple_type *st = new_simple_type(0x12345);
 * simple_type *st1;
 * simple_type *st2;
 * @endcode
 * - When it is destroyed `free_simple_type(st);`
 * - And when a new object is allocated `st1 = new_simple_type(0x54321);`
 * - Then the memory is reused `ASSERT_PTR_EQ(st, st1);`
 * - When another object is allocated `st2 = new_simple_type(0);`
 * - Then the memory is not reused `ASSERT_PTR_NEQ(st2, st1);`
 * - Clean up:
 * @code
 * free_simple_type(st1);
 * free_simple_type(st2);
 * @endcode
 * @test Copy
 * - Given an allocated object
 * @code
 * simple_type *st = new_simple_type(0x12345);
 * simple_type *st1;
 * st->ptr = &st;
 * @endcode
 * - When it is copied `st1 = copy_simple_type(st);`
 * - Then the memory is not shared `ASSERT_PTR_NEQ(st, st1);`
 * - And the contents is copied `ASSERT_PTR_EQ(st1->ptr, st->ptr);`
 * - And the copy hook is executed `ASSERT_UINT_EQ(st1->tag, 0x12346);`
 * - Cleanup:
 * @code
 * free_simple_type(st);
 * free_simple_type(st1);
 * @endcode
 * @test Deallocate and copy
 * - Given two allocated objects
 * @code
 * simple_type *st = new_simple_type(0x12345);
 * simple_type *st1 = new_simple_type(0);
 * simple_type *st2;
 * @endcode
 * - When the first is freed `free_simple_type(st);`
 * - And when the second is copied `st2 = copy_simple_type(st1);`
 * - Then the memory of the first object is reused `ASSERT_PTR_EQ(st2, st);`
 * - Clean up:
 * @code
 * free_simple_type(st1);
 * free_simple_type(st2);
 * @endcode
 * @test
 * Background:
 * @code
 *  typedef short small_type;
 *
 *  DECLARE_TYPE_ALLOCATOR(small_type, ());
 *  DEFINE_TYPE_ALLOCATOR(small_type, (), obj,
 *  {*obj = 0x1234;},
 *  {}, {});
 * @endcode
 * @test Allocate and free small
 * - Given a new small object:
 * @code
 * small_type *sm;
 * small_type *sm1;
 * sm = new_small_type();
 * @endcode
 * - Then it is initialized `ASSERT_INT_EQ(*sm, 0x1234);`
 * - When it is freed `free_small_type(sm);`
 * - And when a new object is allocated `sm1 = new_small_type();`
 * - Then the memory is reused `ASSERT_PTR_EQ(sm, sm1);`
 * - And the second object is initialized `ASSERT_INT_EQ(*sm1, 0x1234);`
 * - Cleanup `free_small_type(sm1);`
 */
#define DECLARE_TYPE_ALLOCATOR(_type, _args)                            \
    extern _type *new_##_type _args                                     \
    ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL;                       \
    extern void free_##_type(_type *_obj);                              \
    extern _type *copy_##_type(const _type *_oldobj)                    \
        ATTR_NONNULL ATTR_RETURNS_NONNULL


/**
 * Generates declarations for freelist-based typed memory allocations
 * which are reference-counted
 * - see DECLARE_TYPE_ALLOCATOR()
 * - `_type *use_<_type>(_type *)`
 *
 * @test Background:
 * @code
 *  typedef struct refcnt_type {
 *    unsigned refcnt;
 *    void *ptr;
 *    unsigned tag;
 *  } refcnt_type;
 *
 * DECLARE_REFCNT_ALLOCATOR(refcnt_type, (unsigned tag));
 * DEFINE_REFCNT_ALLOCATOR(refcnt_type, (unsigned tag), obj,
 *                       {obj->tag = tag;},
 *                       {NEW(obj)->tag = OLD(obj)->tag + 1;},
 *                       {obj->tag = 0xdeadbeef;});
 * @endcode
 * @test Allocate refcounted
 * - Given a fresh refcounted object `refcnt_type *rt = new_refcnt_type(0x12345);`
 * - Then it is allocated `ASSERT_PTR_NEQ(rt, NULL);`
 * - And it is initialized `ASSERT_UINT_EQ(rt->tag, 0x12345);`
 * - And its ref counter is 1 `ASSERT_UINT_EQ(rt->refcnt, 1);`
 * - Cleanup `free_refcnt_type(rt);`
 * @test Allocate and free refcounted
 * - Given a fresh refcounted object `refcnt_type *rt = new_refcnt_type(0x12345);`
 * - When it is destroyed `free_refcnt_type(rt);`
 * - Then it is finalized `ASSERT_UINT_EQ(rt->tag, 0xdeadbeef);`
 * @test  Allocate, use and free refcounted
 * - Given a fresh refcounted object
 * `refcnt_type *rt = new_refcnt_type(0x12345);`
 * - When it is referenced `refcnt_type *use = use_refcnt_type(rt);`
 * - Then the referenced pointer is the same `ASSERT_PTR_EQ(use, rt);`
 * - And the reference counter is incremented by 1 `ASSERT_UINT_EQ(use->refcnt, 2);`
 * - When it is freed `free_refcnt_type(rt);`
 * - Then the reference counter is decremented `ASSERT_UINT_EQ(use->refcnt, 1);`
 * - But the object is not finalized `ASSERT_UINT_EQ(use->tag, 0x12345);`
 * - When the object is freed again `free_refcnt_type(use);`
 * - Then it is finalized `ASSERT_UINT_EQ(use->tag, 0xdeadbeef);`
 * @test Allocate, free and reallocate refcounted
 * - Given a fresh refcounted object
 * @code
 * refcnt_type *rt = new_refcnt_type(0x12345);
 * refcnt_type *rt1;
 * @endcode
 * - When it is freed `free_refcnt_type(rt);`
 * - And when the new object is allocated `rt1 = new_refcnt_type(0x54321);`
 * - Then their pointers are the same `ASSERT_PTR_EQ(rt, rt1);`
 * - And the reference counter is 1 `ASSERT_UINT_EQ(rt1->refcnt, 1);`
 * - And the object is initialized `ASSERT_UINT_EQ(rt1->tag, 0x54321);`
 * - When the copy is freed `free_refcnt_type(rt1);`
 * - Then it is finitalized `ASSERT_UINT_EQ(rt1->tag, 0xdeadbeef);`
 * @test Copy refcounted
 * - Given a fresh refcounted object
 * `refcnt_type *rt = new_refcnt_type(0x12345);`
 * - When it is copied
 * `refcnt_type *rt1 = copy_refcnt_type(rt);`
 * - Then the memory is not shared `ASSERT_PTR_NEQ(rt, rt1);`
 * - And the old object's refcounter is intact `ASSERT_UINT_EQ(rt->refcnt, 1);`
 * - And the new object's refcounter is 1 `ASSERT_UINT_EQ(rt1->refcnt, 1);`
 * - And the new object is initialiozed `ASSERT_UINT_EQ(rt1->tag, 0x12346);`
 * - Cleanup:
 * @code
 * free_refcnt_type(rt1);
 * free_refcnt_type(rt);
 * @endcode
 */
#define DECLARE_REFCNT_ALLOCATOR(_type, _args)                          \
    DECLARE_TYPE_ALLOCATOR(_type, _args);                               \
    static inline _type *use_##_type(_type *val)                        \
    {                                                                   \
        if (!val) return NULL;                                          \
        val->refcnt++;                                                  \
        return val;                                                     \
    }                                                                   \
    struct fake

/** 
 * Generates a declaration for a function to pre-allocate a free-list 
 * - `void preallocate_<_type>s(unsigned size)` 
 *
 * @test Background: 
 * @code 
 * typedef struct simple_type2 { void *ptr; } simple_type2;
 *
 * DECLARE_TYPE_ALLOCATOR(simple_type2, ());
 * DECLARE_TYPE_PREALLOC(simple_type2);
 *
 * DEFINE_TYPE_ALLOCATOR(simple_type2, (), obj,
 * {},
 * {},
 * {});
 * DEFINE_TYPE_PREALLOC(simple_type2);
 * @endcode
 * @test Preallocated items
 * - Given two preallocated items
 * @code
 * simple_type2 *st;
 * simple_type2 *st1;
 * simple_type2 *st2;
 *  
 * preallocate_simple_type2s(2);
 * @endcode
 * - When three objects are allocated
 * @code
 * st = new_simple_type2();
 * st1 = new_simple_type2();
 * st2 = new_simple_type2();
 * @endcode
 * - Then the first two are adjacent in memory
 * @code
 * ASSERT_PTR_EQ(st1, st + 1);
 * ASSERT_PTR_NEQ(st2, st);
 * @endcode
 * - But the last one is not adjacent to any of them
 * `ASSERT_PTR_NEQ(st2, st1);`
 * - Cleanup:
 * @code
 * free_simple_type2(st);
 * free_simple_type2(st1);
 * free_simple_type2(st2));   
 * @endcode
 */
#define DECLARE_TYPE_PREALLOC(_type)                    \
    extern void preallocate_##_type##s(unsigned size)


/** @cond DEV */ 
/**
 * Generate declarations for specific array-handling functions
 */
#define DECLARE_ARRAY_ALLOC_COMMON(_type)                               \
    extern _type *resize_##_type(_type *arr, unsigned newn)             \
        ATTR_RETURNS_NONNULL ATTR_WARN_UNUSED_RESULT;                   \
                                                                        \
    ATTR_NONNULL_1ST ATTR_RETURNS_NONNULL ATTR_WARN_UNUSED_RESULT       \
    static inline _type *ensure_##_type##_size(_type *_arr,             \
                                               unsigned req,            \
                                               unsigned delta)          \
    {                                                                   \
        if (_arr->nelts >= req)                                         \
            return  _arr;                                               \
        return resize_##_type(_arr, req + delta);                       \
    }                                                                   \
    struct fake
/** @endcond */


/**
 * Generate declarations for free-list based array allocations 
 *
 * @test Background:
 * @code
 *  enum simple_array_state {
 *    SA_INITIALIZED = 0xabc,
 *    SA_CLONED = 0xdef,
 *    SA_ADJUSTED = 0x10000000,
 *    SA_RESIZED = 0x999,
 *    SA_FINALIZED = 0xdeadbeef
 *  };
 * 
 *  DECLARE_ARRAY_TYPE(simple_array, void *ptr; unsigned tag;, unsigned);
 *  DECLARE_ARRAY_ALLOCATOR(simple_array);
 * 
 * 
 *  DEFINE_ARRAY_ALLOCATOR(simple_array, linear, 4, arr, i,
 *                        {arr->tag = SA_INITIALIZED;},
 *                        {arr->elts[i] = i; },
 *                        {NEW(arr)->tag = SA_CLONED; },
 *                        {NEW(arr)->elts[i] = OLD(arr)->elts[i] + 1; },
 *                        {NEW(arr)->tag = SA_ADJUSTED; },
 *                        {NEW(arr)->elts[i] = OLD(arr)->elts[i] | SA_ADJUSTED; },
 *                        {arr->tag |= SA_RESIZED; },
 *                        {arr->tag = SA_FINALIZED; },
 *                        {arr->elts[i] = SA_FINALIZED; });
 *
 * static void test_resize_smaller_n(unsigned n)
 * {
 *     simple_array *arr = new_simple_array(n);
 *     simple_array *arr1 = resize_simple_array(arr, n - 1);
 *     unsigned j;
 * 
 *     ASSERT_PTR_EQ(arr, arr1);
 *     ASSERT_UINT_EQ(arr->nelts, n - 1);
 *     ASSERT_BITS_EQ(arr->tag, SA_RESIZED);
 *     for(j = 0; j < n - 1; j++)
 *         ASSERT_UINT_EQ(arr->elts[j], j);
 *     ASSERT_BITS_EQ(arr->elts[n - 1], SA_FINALIZED);
 * 
 *     free_simple_array(arr);
 * }
 * 
 * static void test_resize_larger_n(unsigned n)
 * {
 *     simple_array *arr = new_simple_array(n);
 *     simple_array *arr1 = resize_simple_array(arr, n + 1);
 *     unsigned j;
 * 
 *     ASSERT_PTR_NEQ(arr, arr1);
 *     ASSERT_UINT_EQ(arr1->nelts, n + 1);
 *     ASSERT_BITS_EQ(arr1->tag, SA_ADJUSTED | SA_RESIZED);
 *     for (j = 0; j < n; j++)
 *        ASSERT_BITS_EQ(arr1->elts[j], j | SA_ADJUSTED);
 *     ASSERT_UINT_EQ(arr1->elts[n], n);
 * 
 *     free_simple_array(arr1);
 * }
 * @endcode
 * @test Allocate
 * `simple_array *prev = NULL;`
 * |`unsigned` `size`|
 * |-----------------|
 * | 0               |
 * | 1               |
 * | 2               |
 * | 3               |
 * | 4               |
 * - Given an allocated array of a given size
 * @code
 * simple_array *arr = new_simple_array(i);
 * unsigned j;
 * @endcode
 * - Then it is allocated `ASSERT_PTR_NEQ(arr, NULL);`
 * - And the memory is not shared with an array of lesser size `ASSERT_PTR_NEQ(arr, prev);`
 * - And it is initialized `ASSERT_UINT_EQ(arr->tag, SA_INITIALIZED);`
 * - And the number of elements is set correctly `ASSERT_UINT_EQ(arr->nelts, size);`
 * - And the elements are initialized `for (j = 0; j < size; j++) ASSERT_UINT_EQ(arr->elts[j], j);`
 * - When it is destroyed `free_simple_array(arr);`
 * - When the array is allocated through free-lists `if (size != 4)`
 *   + Then it is finalized `ASSERT_UINT_EQ(arr->tag, SA_FINALIZED);`
 *   + And the elements are finalized: `for(j = 0; j < size; j++)  ASSERT_UINT_EQ(arr->elts[j], SA_FINALIZED);`
 * - Cleanup `prev = arr;`
 * @test 
 * @code
 TESTCASE(
    FORALL(i, 0, 4,
        simple_array *arr = new_simple_array(i);
        simple_array *arr1;

        free_simple_array(arr);
        arr1 = new_simple_array(i);
        ASSERT_PTR_EQ(arr, arr1);
        free_simple_array(arr1);
    ));
 * @endcode
 * @test
 * @code
 TESTCASE(copy_sizes, 
    FORALL(i, 0, 5,
        simple_array *arr = new_simple_array(i);
        simple_array *arr1 = copy_simple_array(arr);

        ASSERT_PTR_NEQ(arr, arr1);
        ASSERT_UINT_EQ(arr1->tag, arr->tag + 1);
        FORALL (j, 0, i,
            ASSERT_UINT_EQ(arr1->elts[j], arr->elts[j] + 1));
        free_simple_array(arr1);
        ASSERT_UINT_EQ(arr->tag, 0x12345);
        free_simple_array(arr);
    ));
 * @endcode
 * @test
 * @code
 TESTCASE(resize_null, 
    simple_array *arr = resize_simple_array(NULL, 3);

    ASSERT_PTR_NEQ(arr, NULL);
    ASSERT_UINT_EQ(arr->nelts, 3);
    ASSERT_UINT_EQ(arr->tag, 0x12345);
    ASSERT_UINT_EQ(arr->elts[2], 2);

    free_simple_array(arr));
 * @endcode
 * @test
 * @code
 TESTCASE(resize_smaller,
    test_resize_smaller_n(3);
    test_resize_smaller_n(4);
    test_resize_smaller_n(5));
 * @endcode
 * @test
 * @code
 TESTCASE(resize_larger,
    test_resize_larger_n(2);
    test_resize_larger_n(3);
    test_resize_larger_n(4));
 * @endcode
 * @test
 * @code
 TESTCASE(resize_larger_free,
    simple_array *arr = new_simple_array(2);
    simple_array *arr1 = resize_simple_array(arr, 3);
    simple_array *arr2 = new_simple_array(2);

    ASSERT_PTR_EQ(arr2, arr);
    free_simple_array(arr1);
    free_simple_array(arr2));
 * @endcode

   );
* @endcode
 * @test
 * @code
   TESTCASE(alloc_log2sizes,
    unsigned i;
    simple_log_array *prev = NULL;

    for (i = 0; i < 5; i++)
    {
        simple_log_array *arr = new_simple_log_array(1u << i);

        ck_assert_ptr_ne(arr, NULL);
        ck_assert_ptr_ne(arr, prev);
        ck_assert_uint_eq(arr->tag, 0x12345);
        ck_assert_uint_eq(arr->nelts, 1u << i);

        free_simple_log_array(arr);
        if (i != 4)
        {
            ck_assert_uint_eq(arr->tag, 0xdeadbeef);
        }
        prev = arr;
    }

   );
* @endcode
 * @test
 * @code
   TESTCASE(alloc_free_log2sizes,
    unsigned i;

    for (i = 2; i < 4; i++)
    {
        simple_log_array *arr = new_simple_log_array(1u << i);
        simple_log_array *arr1;

        free_simple_log_array(arr);
        arr1 = new_simple_log_array((1u << i) - 1);
        ck_assert_ptr_eq(arr, arr1);
        free_simple_log_array(arr1);
    }

   );
* @endcode
 * @test
 * @code
   TESTCASE(resize_larger_log2,
    simple_log_array *arr = new_simple_log_array(9);
    simple_log_array *arr1 = resize_simple_log_array(arr, 10);

    ASSERT_PTR_EQ(arr, arr1);
    ASSERT_UINT_EQ(arr1->nelts, 10);
    ASSERT_UINT_EQ(arr1->elts[9], 9);

    free_simple_log_array(arr1);


  @endcode
 */
#define DECLARE_ARRAY_ALLOCATOR(_type)              \
    DECLARE_TYPE_ALLOCATOR(_type, (unsigned n));    \
    DECLARE_ARRAY_ALLOC_COMMON(_type)       

/**
 * Generate declarations for free-list based array allocations
 * which are reference-counted
 */
#define DECLARE_REFCNT_ARRAY_ALLOCATOR(_type)           \
    DECLARE_REFCNT_ALLOCATOR(_type, (unsigned n));      \
    DECLARE_ARRAY_ALLOC_COMMON(_type)

/**
 * Generate a declaration for an array usable with
 * DECLARE_ARRAY_ALLOCATOR() and DECLARE_REFCNT_ARRAY_ALLOCATOR()
 */
#define DECLARE_ARRAY_TYPE(_name, _contents, _eltype)   \
    typedef struct _name                                \
    {                                                   \
        unsigned nelts;                                 \
        _contents                                       \
        _eltype elts[];                                 \
    } _name
    

#if defined(IMPLEMENT_ALLOCATOR) || defined(__DOXYGEN__)

/** @cond DEV */

/**
 * Generic free list
 */
typedef struct freelist_t {
    struct freelist_t * chain; /**< Next free item */
} freelist_t;

/** @endcond */

/**
 * A helpers to reference old objects in clone and resize handlers
 */
#define OLD(_var) old_##_var

/**
 * A helpers to reference old objects in clone and resize handlers
 */
#define NEW(_var) new_##_var

/** @cond DEV */
/**
 * Generates definitions for allocator functions declared per
 * DECLARE_TYPE_ALLOCATOR() and family
 * 
 */
#define DEFINE_TYPE_ALLOC_COMMON(_type, _args, _var, _init, _clone,     \
                                 _destructor, _fini)                    \
    static freelist_t *freelist_##_type;                                \
                                                                        \
    ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL            \
    static _type *alloc_##_type(void)                                   \
    {                                                                   \
        _type *_var;                                                    \
                                                                        \
        if (freelist_##_type == NULL)                                   \
            _var = frlmalloc(sizeof(*_var));                               \
        else                                                            \
        {                                                               \
            _var = (_type *)freelist_##_type;                           \
            freelist_##_type = freelist_##_type->chain;                 \
        }                                                               \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    _type *new_##_type _args                                            \
    {                                                                   \
        _type *_var = alloc_##_type();                                  \
        _init;                                                          \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    _type *copy_##_type(const _type *OLD(_var))                         \
    {                                                                   \
        _type *NEW(_var) = alloc_##_type();                             \
                                                                        \
        assert(OLD(_var) != NULL);                                      \
        memcpy(NEW(_var), OLD(_var), sizeof(*OLD(_var)));               \
        _clone;                                                         \
        return NEW(_var);                                               \
    }                                                                   \
                                                                        \
    _destructor(_type *_var)                                            \
    {                                                                   \
        if (!_var) return;                                              \
        _fini;                                                          \
        ((freelist_t *)_var)->chain = freelist_##_type;                 \
        freelist_##_type = (freelist_t *)_var;                          \
    }                                                                   \
    struct fake

/** @endcond */


/** 
 * Generates definitions for allocator functions declared per
 * DECLARE_TYPE_ALLOCATOR()
 */
#define DEFINE_TYPE_ALLOCATOR(_type, _args, _var, _init, _clone, _fini) \
    DEFINE_TYPE_ALLOC_COMMON(_type, _args, _var, _init, _clone,         \
                             void free_##_type, _fini)

/** @cond DEV */
/**
 * Generates a reference-counting `free` function
 */
#define DEFINE_REFCNT_FREE(_type, _var)                                 \
    void free_##_type(_type *_var)                                      \
    {                                                                   \
        if (!_var) return;                                              \
        assert(_var->refcnt != 0);                                      \
        if (--_var->refcnt == 0)                                        \
            destroy_##_type(_var);                                      \
    }                                                                   \
    struct fake
/** @endcond */


/** 
 * Generates definitions for allocator functions declared per
 * DECLARE_REFCNT_ALLOCATOR()
 */
#define DEFINE_REFCNT_ALLOCATOR(_type, _args, _var, _init, _clone, _fini) \
    DEFINE_TYPE_ALLOC_COMMON(_type, _args, _var,                        \
                             { _var->refcnt = 1; _init; },              \
                             { NEW(_var)->refcnt = 1; _clone; },        \
                             static inline void destroy_##_type, _fini); \
    DEFINE_REFCNT_FREE(_type, _var)

/** 
 * Generates definition for the preallocated declared by
 * DECLARE_TYPE_PREALLOC()
 */
#define DEFINE_TYPE_PREALLOC(_type)                                     \
    void preallocate_##_type##s(unsigned size)                          \
    {                                                                   \
        unsigned i;                                                     \
        _type *objs = malloc(size * sizeof(*objs));                     \
                                                                        \
        assert(freelist_##_type == NULL);                               \
        assert(sizeof(*objs) >= sizeof(freelist_t));                    \
        for (i = 0; i < size - 1; i++)                                  \
        {                                                               \
            ((freelist_t *)&objs[i])->chain = (freelist_t *)&objs[i + 1]; \
        }                                                               \
        ((freelist_t *)&objs[size - 1])->chain = NULL;                  \
        freelist_##_type = (freelist_t *)objs;                          \
    }    

/** @cond DEV */
/** 
 * Generates definitions for allocator functions declared per
 * DECLARE_ARRAY_ALLOCATOR() and family
 */
#define DEFINE_ARRAY_ALLOC_COMMON(_type, _scale, _maxsize, _var, _idxvar, \
                                  _initc, _inite,                       \
                                  _clonec, _clonee,                      \
                                  _adjustc, _adjuste, _resizec,         \
                                  _destructor, _finic, _finie)          \
  static freelist_t *freelists_##_type[_maxsize];                       \
                                                                        \
  ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL              \
  static _type *alloc_##_type(unsigned n)                               \
  {                                                                     \
      _type *_var;                                                      \
      unsigned _sz = _scale##_order(n);                                 \
                                                                        \
      if (_sz >= _maxsize || freelists_##_type[_sz] == NULL)            \
          _var = frlmalloc(sizeof (*_var) + _scale##_size(_sz) *        \
                           sizeof(_var->elts[0]));                      \
      else                                                              \
      {                                                                 \
          _var = (_type *)freelists_##_type[_sz];                       \
          freelists_##_type[_sz] = freelists_##_type[_sz]->chain;       \
      }                                                                 \
      _var->nelts = n;                                                  \
      return _var;                                                      \
  }                                                                     \
                                                                        \
  _type *new_##_type(unsigned _n)                                       \
  {                                                                     \
      _type *_var = alloc_##_type(_n);                                  \
      unsigned _idxvar;                                                 \
                                                                        \
      for (_idxvar = 0; _idxvar < _n; _idxvar++)                        \
      {                                                                 \
          _inite;                                                       \
      }                                                                 \
      _initc;                                                           \
      return _var;                                                      \
  }                                                                     \
                                                                        \
  _type *copy_##_type(const _type *OLD(_var))                           \
  {                                                                     \
      _type *NEW(_var) = alloc_##_type(OLD(_var)->nelts);               \
      unsigned _idxvar;                                                 \
                                                                        \
      memcpy(NEW(_var), OLD(_var),                                      \
             sizeof(_type) +                                            \
             OLD(_var)->nelts * sizeof(OLD(_var)->elts[0]));            \
      for (_idxvar = 0; _idxvar < OLD(_var)->nelts; _idxvar++)          \
      {                                                                 \
          _clonee;                                                      \
      }                                                                 \
      _clonec;                                                          \
      return NEW(_var);                                                 \
  }                                                                     \
                                                                        \
  ATTR_NONNULL                                                          \
  static void dispose_##_type(_type *_var)                              \
  {                                                                     \
      unsigned _sz = _scale##_order(_var->nelts);                       \
                                                                        \
      if(_sz >= _maxsize)                                               \
          free(_var);                                                   \
      else                                                              \
      {                                                                 \
          ((freelist_t *)_var)->chain = freelists_##_type[_sz];         \
          freelists_##_type[_sz] = (freelist_t *)_var;                  \
      }                                                                 \
  }                                                                     \
                                                                        \
  _type *resize_##_type(_type *_var, unsigned _newn)                    \
  {                                                                     \
    unsigned _idxvar;                                                   \
                                                                        \
    if (_var == NULL)                                                   \
        return new_##_type(_newn);                                      \
                                                                        \
    for (_idxvar = _newn; _idxvar < _var->nelts; _idxvar++)             \
    {                                                                   \
        _finie;                                                         \
    }                                                                   \
                                                                        \
    if (_scale##_order(_newn) > _scale##_order(_var->nelts))            \
    {                                                                   \
        const _type *OLD(_var) = _var;                                  \
        _type *NEW(_var) = alloc_##_type(_newn);                        \
                                                                        \
        memcpy(NEW(_var), OLD(_var),                                    \
               sizeof(_type) +                                          \
               OLD(_var)->nelts * sizeof(OLD(_var)->elts[0]));          \
        for (_idxvar = 0; _idxvar < NEW(_var)->nelts; _idxvar++)        \
        {                                                               \
            _adjuste;                                                   \
        }                                                               \
        _adjustc;                                                       \
        dispose_##_type(_var);                                          \
        _var = NEW(_var);                                               \
    }                                                                   \
                                                                        \
    for (_idxvar = _var->nelts; _idxvar < _newn; _idxvar++)             \
    {                                                                   \
        _inite;                                                         \
    }                                                                   \
    _var->nelts = _newn;                                                \
    _resizec;                                                           \
    return _var;                                                        \
  }                                                                     \
                                                                        \
  _destructor(_type *_var)                                              \
  {                                                                     \
      unsigned _idxvar;                                                 \
                                                                        \
      for (_idxvar = 0; _idxvar < _var->nelts; _idxvar++)               \
      {                                                                 \
          _finie;                                                       \
      }                                                                 \
      _finic;                                                           \
      dispose_##_type(_var);                                            \
  }                                                                     \
  struct fake
/** @endcond */


/**
 * Like `malloc`, but ensures that allocated block will be able to hold
 * freelist_t object
 */
ATTR_MALLOC ATTR_RETURNS_NONNULL
static inline void *frlmalloc(size_t sz) 
{
    void *result = malloc(sz > sizeof(freelist_t) ? sz : sizeof(freelist_t));
    assert(result != NULL);
    return result;
}

#define linear_order(_x) (_x)
#define linear_size(_x) (_x)

/**
 * @test log2order
 * @p x | Result
 * -------------
 * 0    | 0
 * 1    | 0
 * 2    | 1
 * 3    | 2
 * 4    | 2
 * 5    | 3
 * 65536| 16
 */
static_inline unsigned log2_order(unsigned x)
{
    return x ? ((unsigned)sizeof(unsigned) * CHAR_BIT -
                count_leading_zeroes(x - 1)) : 0;
}
/** 
 * The inverse of log2_order()
 */
#define log2_size(_x) (1u << (_x))

/**
 * Defines a linear allocation sc
 */
#define DEFINE_LINEAR_SCALE(_n)                                 \
    static inline unsigned linear##_n##_order(unsigned x)       \
    {                                                           \
        return x / (_n);                                        \
    }                                                           \
                                                                \
    static inline unsigned linear##_n##_size(unsigned order)    \
    {                                                           \
        return (order + 1) * (_n);                              \
    }                                                           \
    struct fake


#define DEFINE_ARRAY_ALLOCATOR(_type, _scale, _maxsize, _var, _idxvar,  \
                               _initc, _inite,                          \
                               _clonec, _clonee,                        \
                               _adjustc, _adjuste, _resizec,            \
                               _finic, _finie)                          \
    DEFINE_ARRAY_ALLOC_COMMON(_type, _scale, _maxsize, _var, _idxvar,   \
                              _initc, _inite,                           \
                              _clonec, _clonee,                         \
                              _adjustc, _adjuste, _resizec,             \
                              void free_##_type, _finic, _finie)

#define DEFINE_REFCNT_ARRAY_ALLOCATOR(_type, _scale, _maxsize, _var,    \
                                      _idxvar,                          \
                                      _initc, _inite,                   \
                                      _clonec, _clone,                  \
                                      _adjustc, _adjuste, _resizec,     \
                                      _finic, _finie)                   \
    DEFINE_ARRAY_ALLOC_COMMON(_type, _scale, _maxsize, _var, _idxvar,   \
                              { _var->refcnt = 1; _initc}, _inite,      \
                              { NEW(_var)->refcnt = 1; _clonec}, _clone, \
                              _adjustc, _adjuste,                       \
                              {assert(_var->refcnt == 1); _resizec},    \
                              static inline void destroy_##_type,       \
                              _finic, _finie);                          \
    DEFINE_REFCNT_FREE(_type, _var)

#endif // defined(ALLOCATOR_IMP)
  
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ALLOCATOR_H */
