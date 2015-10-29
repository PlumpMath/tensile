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
 * #define TRACK_ALLOCATOR 1
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
 *  enum object_state {
 *    STATE_INITIALIZED = 0xb1ff,
 *    STATE_CLONED = 0x20000000,
 *    STATE_ADJUSTED = 0x10000000,
 *    STATE_RESIZED = 0xc001,
 *    STATE_FINALIZED = 0xdead
 *  };
 * typedef struct simple_type {
 *    void *ptr;
 *    unsigned tag;
 * } simple_type;
 *
 * DECLARE_TYPE_ALLOCATOR(simple_type, (unsigned tag));
 *
 * DEFINE_TYPE_ALLOCATOR(simple_type, (unsigned tag), obj,
 * { obj->tag = tag; },
 * { NEW(obj)->tag = OLD(obj)->tag | STATE_CLONED; },
 * { obj->tag = STATE_FINALIZED; });
 * @endcode
 *
 * @test Allocate
 * - Given a fresh object 
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * simple_type *st = new_simple_type(thetag);
 * @endcode
 * - Then it is allocated `ASSERT_PTR_NEQ(st, NULL);`
 * - And it is initialized `ASSERT_UINT_EQ(st->tag, thetag);`
 * - Clean up `free_simple_type(st);`
 *
 * @test Allocate and free
 * - Given an allocated object 
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * simple_type *st = new_simple_type(thetag);
 * @endcode
 * - When it is destroyed `free_simple_type(st);`
 * - Then the free list is in proper state `ASSERT_PTR_EQ(freelist_simple_type, st);`
 * - And the object is finalized `ASSERT_UINT_EQ(st->tag, STATE_FINALIZED);`
 * - And the allocated object count is zero:
 *   `ASSERT_UINT_EQ(track_alloc_simple_type, 0);`
 *
 * @test Allocate, free and reallocate
 * - Given an allocated object
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * unsigned thetag2 = ARBITRARY(unsigned, 1, 0xfffff);
 * simple_type *st = new_simple_type(thetag);
 * simple_type *st1;
 * @endcode
 * - When it is destroyed `free_simple_type(st);`
 * - And when another object is allocated `st1 = new_simple_type(thetag2);`
 * - Then the memory is reused `ASSERT_PTR_EQ(st, st1);`
 * - And the free list is in proper state `ASSERT_PTR_EQ(freelist_simple_type, NULL);`
 * - And the new object is initialized `ASSERT_UINT_EQ(st1->tag, thetag2);`
 * - And the allocated object count is one: 
 *   `ASSERT_UINT_EQ(track_alloc_simple_type, 1);`
 * - Clean up `free_simple_type(st1);`
 *
 * @test Allocate, free, reallocate and allocate another
 * - Given an allocated object
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * unsigned thetag2 = ARBITRARY(unsigned, 1, 0xfffff);
 * simple_type *st = new_simple_type(thetag);
 * simple_type *st1;
 * simple_type *st2;
 * @endcode
 * - When it is destroyed `free_simple_type(st);`
 * - And when a new object is allocated `st1 = new_simple_type(thetag2);`
 * - Then the memory is reused `ASSERT_PTR_EQ(st, st1);`
 * - When another object is allocated `st2 = new_simple_type(0);`
 * - Then the memory is not reused `ASSERT_PTR_NEQ(st2, st1);`
 * - And the allocated object count is two: 
 *   `ASSERT_UINT_EQ(track_alloc_simple_type, 2);`
 * - Clean up:
 * @code
 * free_simple_type(st1);
 * free_simple_type(st2);
 * ASSERT_UINT_EQ(track_alloc_simple_type, 0);
 * @endcode
 *
 * @test Copy
 * - Given an allocated object
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * simple_type *st = new_simple_type(thetag);
 * simple_type *st1;
 * st->ptr = &st;
 * @endcode
 * - When it is copied `st1 = copy_simple_type(st);`
 * - Then the memory is not shared `ASSERT_PTR_NEQ(st, st1);`
 * - And the contents is copied `ASSERT_PTR_EQ(st1->ptr, st->ptr);`
 * - And the copy hook is executed `ASSERT_UINT_EQ(st1->tag, thetag | STATE_CLONED);`
 * - And the allocated object count is two:
 *   `ASSERT_UINT_EQ(track_alloc_simple_type, 2);`
 * - Cleanup:
 * @code
 * free_simple_type(st);
 * free_simple_type(st1);
 * ASSERT_UINT_EQ(track_alloc_simple_type, 0);
 * @endcode
 *
 * @test Deallocate and copy
 * - Given two allocated objects
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * unsigned thetag2 = ARBITRARY(unsigned, 1, 0xfffff);
 * simple_type *st = new_simple_type(thetag);
 * simple_type *st1 = new_simple_type(thetag2);
 * simple_type *st2;
 * @endcode
 * - When the first is freed `free_simple_type(st);`
 * - And when the second is copied `st2 = copy_simple_type(st1);`
 * - Then the memory of the first object is reused `ASSERT_PTR_EQ(st2, st);`
 * - Clean up:
 * @code
 * free_simple_type(st1);
 * free_simple_type(st2);
 * ASSERT_UINT_EQ(track_alloc_simple_type, 0);
 * @endcode
 *
 * @test
 * Background:
 * @code
 *  typedef short small_type;
 *
 *  DECLARE_TYPE_ALLOCATOR(small_type, ());
 *  DEFINE_TYPE_ALLOCATOR(small_type, (), obj,
 *  {*obj = (short)STATE_INITIALIZED;},
 *  {}, {});
 * @endcode
 *
 * @test Allocate and free small
 * - Given a new small object:
 * @code
 * small_type *sm;
 * small_type *sm1;
 * sm = new_small_type();
 * @endcode
 * - Then it is initialized `ASSERT_INT_EQ(*sm, (short)STATE_INITIALIZED);`
 * - When it is freed `*sm = 0; free_small_type(sm);`
 * - And when a new object is allocated `sm1 = new_small_type();`
 * - Then the memory is reused `ASSERT_PTR_EQ(sm, sm1);`
 * - And the second object is initialized `ASSERT_INT_EQ(*sm1, (short)STATE_INITIALIZED);`
 * - Cleanup `free_small_type(sm1);`
 */
#define DECLARE_TYPE_ALLOCATOR(_type, _args)                            \
    GENERATED_DECL _type *new_##_type _args                             \
    ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL;                       \
    GENERATED_DECL void free_##_type(_type *_obj);                      \
    GENERATED_DECL _type *copy_##_type(const _type *_oldobj)            \
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
 *                       {NEW(obj)->tag = OLD(obj)->tag | STATE_CLONED;},
 *                       {obj->tag = STATE_FINALIZED;});
 * @endcode
 *
 * @test Allocate refcounted
 * - Given a fresh refcounted object 
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * @endcode
 * - Then it is allocated `ASSERT_PTR_NEQ(rt, NULL);`
 * - And it is initialized `ASSERT_UINT_EQ(rt->tag, thetag);`
 * - And its ref counter is 1 `ASSERT_UINT_EQ(rt->refcnt, 1);`
 * - And the allocated object count is one: 
 *   `ASSERT_UINT_EQ(track_alloc_refcnt_type, 1);`
 * - Cleanup `free_refcnt_type(rt);`
 *
 * @test Allocate and free refcounted
 * - Given a fresh refcounted object 
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * @endcode
 * - When it is destroyed `free_refcnt_type(rt);`
 * - Then it is finalized `ASSERT_UINT_EQ(rt->tag, STATE_FINALIZED);`
 * - And the allocated object count is zero:
 *   `ASSERT_UINT_EQ(track_alloc_refcnt_type, 0);` 
 *
 * @test  Allocate, use and free refcounted
 * - Given a fresh refcounted object
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * @endcode
 * - When it is referenced `refcnt_type *use = use_refcnt_type(rt);`
 * - Then the referenced pointer is the same `ASSERT_PTR_EQ(use, rt);`
 * - And the reference counter is incremented by 1 `ASSERT_UINT_EQ(use->refcnt, 2);`
 * - When it is freed `free_refcnt_type(rt);`
 * - Then the reference counter is decremented `ASSERT_UINT_EQ(use->refcnt, 1);`
 * - But the object is not finalized `ASSERT_UINT_EQ(use->tag, thetag);`
 * - When the object is freed again `free_refcnt_type(use);`
 * - Then it is finalized `ASSERT_UINT_EQ(use->tag, STATE_FINALIZED);`
 *
 * @test Allocate, free and reallocate refcounted
 * - Given a fresh refcounted object
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * unsigned thetag2 = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * refcnt_type *rt1;
 * @endcode
 * - When it is freed `free_refcnt_type(rt);`
 * - And when the new object is allocated `rt1 = new_refcnt_type(thetag2);`
 * - Then their pointers are the same `ASSERT_PTR_EQ(rt, rt1);`
 * - And the reference counter is 1 `ASSERT_UINT_EQ(rt1->refcnt, 1);`
 * - And the object is initialized `ASSERT_UINT_EQ(rt1->tag, thetag2);`
 * - When the copy is freed `free_refcnt_type(rt1);`
 * - Then it is finitalized `ASSERT_UINT_EQ(rt1->tag, STATE_FINALIZED);`
 *
 * @test Copy refcounted
 * - Given a fresh refcounted object
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * @endcode
 * - When it is copied
 * `refcnt_type *rt1 = copy_refcnt_type(rt);`
 * - Then the memory is not shared `ASSERT_PTR_NEQ(rt, rt1);`
 * - And the old object's refcounter is intact `ASSERT_UINT_EQ(rt->refcnt, 1);`
 * - And the new object's refcounter is 1 `ASSERT_UINT_EQ(rt1->refcnt, 1);`
 * - And the new object is initialiazed `ASSERT_UINT_EQ(rt1->tag, thetag | STATE_CLONED);`
 * - Cleanup:
 * @code
 * free_refcnt_type(rt1);
 * free_refcnt_type(rt);
 * ASSERT_UINT_EQ(track_alloc_refcnt_type, 0);
 * @endcode
 */
#define DECLARE_REFCNT_ALLOCATOR(_type, _args)                          \
    DECLARE_TYPE_ALLOCATOR(_type, _args);                               \
                                                                        \
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
 *
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
 * free_simple_type2(st2);   
*  ASSERT_UINT_EQ(track_alloc_simple_type2, 0);
 * @endcode
 */
#define DECLARE_TYPE_PREALLOC(_type)                            \
    GENERATED_DECL void preallocate_##_type##s(unsigned size)


/** @cond DEV */ 
/**
 * Generate declarations for specific array-handling functions
 */
#define DECLARE_ARRAY_ALLOC_COMMON(_name, _eltype)                      \
    GENERATED_DECL _type *resize_##_name(_type **arr,                   \
                                       size_t *oldn, size_t newn)       \
        ATTR_RETURNS_NONNULL ATTR_WARN_UNUSED_RESULT;                   \
                                                                        \
    ATTR_NONNULL_1ST ATTR_RETURNS_NONNULL ATTR_WARN_UNUSED_RESULT       \
    static inline _type *ensure_##_type##_size(_type **_arr,            \
                                               size_t *oldn,            \
                                               size_t req,              \
                                               size_t delta)            \
    {                                                                   \
        if (*oldn >= req)                                               \
            return  _arr;                                               \
        return resize_##_type(_arr, oldn, req + delta);                 \
    }                                                                   \
    struct fake
/** @endcond */


/**
 * Generate declarations for free-list based array allocations 
 *
 * @test Background:
 * @code
 * 
 *  DECLARE_ARRAY_TYPE(simple_array, void *ptr; unsigned tag;, unsigned);
 *  DECLARE_ARRAY_ALLOCATOR(simple_array);
 * 
 * 
 *  DEFINE_ARRAY_ALLOCATOR(simple_array, linear, 4, arr, i,
 *                        {arr->tag = STATE_INITIALIZED;},
 *                        {arr->elts[i] = i; },
 *                        {NEW(arr)->tag = STATE_CLONED; },
 *                        {NEW(arr)->elts[i] = OLD(arr)->elts[i] | STATE_CLONED; },
 *                        {NEW(arr)->tag = STATE_ADJUSTED; },
 *                        {NEW(arr)->elts[i] = OLD(arr)->elts[i] | STATE_ADJUSTED; },
 *                        {arr->tag |= STATE_RESIZED; },
 *                        {arr->tag = STATE_FINALIZED; },
 *                        {arr->elts[i] = STATE_FINALIZED; });
 *
 * static void test_resize_smaller_n(unsigned n)
 * {
 *     simple_array *arr = new_simple_array(n);
 *     simple_array *arr1 = resize_simple_array(arr, n - 1);
 *     unsigned j;
 * 
 *     ASSERT_PTR_EQ(arr, arr1);
 *     ASSERT_UINT_EQ(arr->nelts, n - 1);
 *     ASSERT_BITS_EQ(arr->tag, STATE_INITIALIZED | STATE_RESIZED);
 *     for (j = 0; j < n - 1; j++)
 *         ASSERT_UINT_EQ(arr->elts[j], j);
 *     ASSERT_BITS_EQ(arr->elts[n - 1], STATE_FINALIZED);
 * 
 *     free_simple_array(arr);
 *     for (j = 0; j <= 4; j++)
 *         ASSERT_UINT_EQ(track_alloc_simple_array[j], 0);
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
 *     ASSERT_BITS_EQ(arr1->tag, STATE_ADJUSTED | STATE_RESIZED);
 *     for (j = 0; j < n; j++)
 *        ASSERT_BITS_EQ(arr1->elts[j], j | STATE_ADJUSTED);
 *     ASSERT_UINT_EQ(arr1->elts[n], n);
 * 
 *     free_simple_array(arr1);
 *     for (j = 0; j <= 4; j++)
 *         ASSERT_UINT_EQ(track_alloc_simple_array[j], 0);
 * }
 * @endcode
 *
 * @test Allocate and free
 * | Varying                  | From|To |
 * |--------------------------|-----|---|
 * | @testvar{unsigned,size,u}|`0`  |`4`|
 * `simple_array *prev = NULL;`
 * - Given an allocated array of a given size
 * @code
 * simple_array *arr = new_simple_array(size);
 * unsigned j;
 * @endcode
 * - Then it is allocated `ASSERT_PTR_NEQ(arr, NULL);`
 * - And the memory is not shared with an array of lesser size:
 *   `ASSERT_PTR_NEQ(arr, prev);`
 * - And the allocated object counter is 1:
 *   `ASSERT_UINT_EQ(track_alloc_simple_array[size], 1);`
 * - And it is initialized `ASSERT_UINT_EQ(arr->tag, STATE_INITIALIZED);`
 * - And the number of elements is set correctly `ASSERT_UINT_EQ(arr->nelts, size);`
 * - And the elements are initialized `for (j = 0; j < size; j++) ASSERT_UINT_EQ(arr->elts[j], j);`
 * - When it is destroyed `free_simple_array(arr);`
 * - When the array is allocated through free-lists `if (size != 4)`
 *   + Then it is finalized `ASSERT_UINT_EQ(arr->tag, STATE_FINALIZED);`
 *   + And the elements are finalized: 
 *     @code
 *     for(j = 0; j < size; j++)
 *       ASSERT_UINT_EQ(arr->elts[j], STATE_FINALIZED);
 *     @endcode
 * - And the allocated object count is zero:
 *   `ASSERT_UINT_EQ(track_alloc_simple_array[size], 0);`
 * - Cleanup `prev = arr;`
 *
 * @test Allocate, free and reallocate
 * - Given an allocated array of a given size
 * @code
 * simple_array *arr = new_simple_array(size);
 * simple_array *arr1;
 * @endcode
 * - When it is freed `free_simple_array(arr);`
 * - And when a new array of the same size is allocated `arr1 = new_simple_array(size);`
 * - Then the memory is shared `ASSERT_PTR_EQ(arr, arr1);`
 * - Cleanup: `free_simple_array(arr1); `
 *            `ASSERT_UINT_EQ(track_alloc_simple_array[size], 0);`
 * .
 * | Varying                  | From|To |
 * |--------------------------|-----|---|
 * | @testvar{unsigned,size,u}|`0`  |`3`|
 *
 * @test Copy
 * - Given an array:
 * @code
 * simple_array *arr = new_simple_array(size);
 * unsigned j;
 * @endcode
 * - When it is copied `simple_array *arr1 = copy_simple_array(arr);`
 * - Then the memory is not shared `ASSERT_PTR_NEQ(arr, arr1);`
 * - And the copy is initialiazed `ASSERT_UINT_EQ(arr1->tag, STATE_CLONED);`
 * - And the items are initialized: 
 * @code
 * for(j = 0; j < size; j++) ASSERT_UINT_EQ(arr1->elts[j], arr->elts[j] | STATE_CLONED);
 * @endcode
 * - When the copy is freed `free_simple_array(arr1);`
 * - Then the original is untouched `ASSERT_UINT_EQ(arr->tag, STATE_INITIALIZED);`
 * - Cleanup: `free_simple_array(arr);`
 * .
 * | Varying                  | From|To |
 * |--------------------------|-----|---|
 * | @testvar{unsigned,size,u}|`0`  |`4`|
 *
 * @test Resize Null
 * - When a NULL pointer to an array is resized 
 * @code
 * simple_array *arr = resize_simple_array(NULL, 3);
 * unsigned j;
 * @endcode
 * - Then the result is not NULL `ASSERT_PTR_NEQ(arr, NULL);`
 * - And the number of elements is correct `ASSERT_UINT_EQ(arr->nelts, 3);`
 * - And the array is initialized `ASSERT_UINT_EQ(arr->tag, STATE_INITIALIZED);`
 * - And the elements are initialized `for (j = 0; j < arr->nelts; j++) ASSERT_UINT_EQ(arr->elts[j], j);`
 * - Cleanup `free_simple_array(arr);`
 *
 * @test Verify that resizing to a lesser size works
 * Varying                  | From | To
 * -------------------------|------|----
 * @testvar{unsigned,size,u}| `3`  | `4`
 * @code
 * test_resize_smaller_n(size);
 * @endcode
 *
 * @test Verify that resizing to a larger size works
 * Varying                  | From | To
 * -------------------------|------|----
 * @testvar{unsigned,size,u}| `2`  | `3`
 * @code
 * test_resize_larger_n(size);
 * @endcode
 *
 * @test Resize and Allocate
 * - Given an array
 * @code
 * simple_array *arr = new_simple_array(2);
 * @endcode
 * - When it is resized
 * @code
 * simple_array *arr1 = resize_simple_array(arr, 3);
 * @endcode
 * - And when a new array with the same size as the original is allocated
 * @code
 * simple_array *arr2 = new_simple_array(2);
 * @endcode
 * - Then the memory is reused `ASSERT_PTR_EQ(arr2, arr);`
 * - Cleanup
 * @code
 * free_simple_array(arr1);
 * free_simple_array(arr2);
 * @endcode
 *
 * @test Background:
 * @code
 * DECLARE_ARRAY_TYPE(simple_log_array, void *ptr; unsigned tag;, unsigned);
 * DECLARE_ARRAY_ALLOCATOR(simple_log_array);
 *
 * DEFINE_ARRAY_ALLOCATOR(simple_log_array, log2, 4, arr, i,
 *                      {arr->tag = STATE_INITIALIZED;},
 *                      {arr->elts[i] = i; },
 *                      {NEW(arr)->tag = STATE_CLONED; },
 *                      {NEW(arr)->elts[i] = OLD(arr)->elts[i] | STATE_CLONED; },
 *                      {NEW(arr)->tag = STATE_ADJUSTED; },
 *                      {NEW(arr)->elts[i] = OLD(arr)->elts[i] | STATE_ADJUSTED; },
 *                      {arr->tag |= STATE_RESIZED; },
 *                      {arr->tag = STATE_FINALIZED; },
 *                      {arr->elts[i] = STATE_FINALIZED; });
 * @endcode
 *
 * @test Allocate and free (log scale)
 * | Varying                  | From|To |
 * |--------------------------|-----|---|
 * |@testvar{unsigned,order,u}|`0`  |`3`|
 * `simple_log_array *prev = NULL;`
 * - Given an allocated array of a certain size
 *   `simple_log_array *arr = new_simple_log_array(1u << order);`
 * - Verify it is allocated `ASSERT_PTR_NEQ(arr, NULL);`
 * - Verify the memory is not shared with `ASSERT_PTR_NEQ(arr, prev);`
 * - Verify that it is initialzied `ASSERT_UINT_EQ(arr->tag, STATE_INITIALIZED);`
 * - Verify that the number of elements is correct `ASSERT_UINT_EQ(arr->nelts, 1u << order);`
 * - When it is freed `free_simple_log_array(arr);`
 * - When it is not so large `if (order != 4)`
     + Then it is finalized `ASSERT_UINT_EQ(arr->tag, STATE_FINALIZED);`
 * - Cleanup `prev = arr;`
 *
 * @test Allocate, free and allocate slightly lesser
 * - Given an allocated array of a certain size
 * @code
 * simple_log_array *arr = new_simple_log_array(1u << order);
 * simple_log_array *arr1;
 * @endcode
 * - When it freed `free_simple_log_array(arr);`
 * - And when a new array which is slightly less is allocated
 *   `arr1 = new_simple_log_array((1u << order) - 1);`
 * - Then it shares memory with the first array `ASSERT_PTR_EQ(arr, arr1);`
 * - Cleanup `free_simple_log_array(arr1);`
 * .
 * | Varying                   | From|To |
 * |---------------------------|-----|---|
 * | @testvar{unsigned,order,u}|`2`  |`3`|
 *
 * @test Make slightly larger
 * - Given an allocated array of a certain size
 * `simple_log_array *arr = new_simple_log_array(9);`
 * - When it is enlarged such that log2 size is still the same
 * `simple_log_array *arr1 = resize_simple_log_array(arr, 10);`
 * - Then the memory is not reallocated
 * `ASSERT_PTR_EQ(arr, arr1);`
 * - And the number of elements is set properly
 * `ASSERT_UINT_EQ(arr1->nelts, 10);`
 * - And the extra elements are initialized
 * `ASSERT_UINT_EQ(arr1->elts[9], 9);`
 * - Cleanup `free_simple_log_array(arr1);`
 *
 * @test
 * Ensure Size
 * - Given an allocated array
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * unsigned delta = ARBITRARY(unsigned, 0, 4);
 * simple_array *arr = new_simple_array(sz);
 * simple_array *arr1 = NULL;
 * @endcode
 * - When a lesser size is ensured:
 *   `arr1 = ensure_simple_array_size(arr, sz - 1, 1);`
 * - Then the array is not reallocated:
 *   `ASSERT_PTR_EQ(arr, arr1);`
 * - And the number of elements is unchanged:
 *  `ASSERT_UINT_EQ(arr->nelts, sz);`
 * - When a larger size is ensured
 *   `arr1 = ensure_simple_array_size(arr, sz + 1, delta);`
 * - Then the array is reallocated:
 *   `ASSERT_PTR_NEQ(arr, arr1);`
 * - And the number of elements is incremented:
 *   `ASSERT_UINT_EQ(arr1->nelts, sz + 1 + delta);`
 * - Cleanup: `free_simple_array(arr1);`
 */
#define DECLARE_ARRAY_ALLOCATOR(_type)              \
    DECLARE_TYPE_ALLOCATOR(_type, (unsigned n));    \
    DECLARE_ARRAY_ALLOC_COMMON(_type)       

/**
 * Generate declarations for free-list based array allocations
 * which are reference-counted
 *
 * @test Background:
 * @code
 * DECLARE_ARRAY_TYPE(refcnt_array,
 * void *ptr; unsigned refcnt; unsigned tag;,
 * unsigned);
 *
 * DECLARE_REFCNT_ARRAY_ALLOCATOR(refcnt_array);
 * DEFINE_REFCNT_ARRAY_ALLOCATOR(refcnt_array,
 * linear, 4, arr, i,
 * {arr->tag = STATE_INITIALIZED;},
 * {}, {}, {}, {}, {}, {},
 * {arr->tag = STATE_FINALIZED;}, {});
 * @endcode
 *
 * @test
 * Allocate and free refcounted array
 * - Given a fresh array
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * refcnt_array *rt = new_refcnt_array(sz);
 * @endcode
 * - Verify that it is allocated `ASSERT_PTR_NEQ(rt, NULL);`
 * - Verify that it is initialized 
 *   `ASSERT_UINT_EQ(rt->tag, STATE_INITIALIZED);`
 * - Verify that the number of elements is correct
 *   `ASSERT_UINT_EQ(rt->nelts, sz);`
 * - Verify that the refcounter is 1
 *   `ASSERT_UINT_EQ(rt->refcnt, 1);`
 * - Cleanup: `free_refcnt_array(rt);`
 *
 * @test 
 * Allocate, use and free refcounted array
 * - Given a fresh array
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * refcnt_array *rt = new_refcnt_array(sz); 
 * @endcode
 * - When it is used
 * `refcnt_array *use = use_refcnt_array(rt);`
 * - Then the pointer does not change
 * `ASSERT_PTR_EQ(use, rt);`
 * - And the refcounter is incremented
 * `ASSERT_UINT_EQ(use->refcnt, 2);`
 * - When it is freed:
 * `free_refcnt_array(rt);`
 * - Then the refcounter is decremented
 * `ASSERT_UINT_EQ(use->refcnt, 1);`
 * - But the object is not finalized:
 * `ASSERT_UINT_EQ(use->tag, STATE_INITIALIZED);`
 * - When it is freed again:
 * `free_refcnt_array(use);`
 * - Then it is finalized:
 * `ASSERT_UINT_EQ(use->tag, STATE_FINALIZED);`
 *
 * @test
 * Copy a refcounted array
 * - Given a new refcounted array:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * refcnt_array *rt = new_refcnt_array(sz);
 * @endcode
 * - When it is copied:
 * `refcnt_array *rt1 = copy_refcnt_array(rt);`
 * - Then the memory is not shared `ASSERT_PTR_NEQ(rt, rt1);`
 * - And the refcounter is not changed: `ASSERT_UINT_EQ(rt->refcnt, 1);`
 * - And the refcounter of the copy is 1:
 * `ASSERT_UINT_EQ(rt1->refcnt, 1);`
 * - And the copy is initialized:
 * `ASSERT_UINT_EQ(rt1->tag, STATE_INITIALIZED);`
 * - And the number of elements of the copy is correct
 * `ASSERT_UINT_EQ(rt1->nelts, sz);`
 * - Cleanup:
 * @code
 * free_refcnt_array(rt1);
 * free_refcnt_array(rt);
 * @endcode
 */

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

#if TRACK_ALLOCATOR
#define ALLOC_COUNTER(_name) static size_t track_alloc_##_name
#define ALLOC_COUNTERS(_name, _size) ALLOC_COUNTER(_name)[_size]
#define TRACK_ALLOC(_name) (track_alloc_##_name)++
#define TRACK_ALLOC_IDX(_name, _idx) (track_alloc_##_name[_idx])++
#define TRACK_FREE(_name) (track_alloc_##_name)--
#define TRACK_FREE_IDX(_name, _idx) (track_alloc_##_name[_idx])--
#else
#define ALLOC_COUNTER(_name) struct fake
#define ALLOC_COUNTERS(_name, _size) struct fake
#define TRACK_ALLOC(_name) ((void)0)
#define TRACK_ALLOC_IDX(_name, _idx) ((void)0)
#define TRACK_FREE(_name) ((void)0)
#define TRACK_FREE_IDX(_name, _idx) ((void)0)
#endif

/** @cond DEV */
/**
 * Generates definitions for allocator functions declared per
 * DECLARE_TYPE_ALLOCATOR() and family
 * 
 */
#define DEFINE_TYPE_ALLOC_COMMON(_type, _args, _var, _init, _clone,     \
                                 _destructor, _fini)                    \
    ALLOC_COUNTER(_type);                                               \
    static freelist_t *freelist_##_type;                                \
                                                                        \
    ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL            \
    static _type *alloc_##_type(void)                                   \
    {                                                                   \
        _type *_var;                                                    \
                                                                        \
        if (freelist_##_type == NULL)                                   \
            _var = frlmalloc(sizeof(*_var));                            \
        else                                                            \
        {                                                               \
            _var = (_type *)freelist_##_type;                           \
            freelist_##_type = freelist_##_type->chain;                 \
        }                                                               \
        TRACK_ALLOC(_type);                                             \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    GENERATED_DEF _type *new_##_type _args                              \
    {                                                                   \
        _type *_var = alloc_##_type();                                  \
        _init;                                                          \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    GENERATED_DEF _type *copy_##_type(const _type *OLD(_var))           \
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
        TRACK_FREE(_type);                                              \
    }                                                                   \
    struct fake

/** @endcond */


/** 
 * Generates definitions for allocator functions declared per
 * DECLARE_TYPE_ALLOCATOR()
 */
#define DEFINE_TYPE_ALLOCATOR(_type, _args, _var, _init, _clone, _fini) \
    DEFINE_TYPE_ALLOC_COMMON(_type, _args, _var, _init, _clone,         \
                             GENERATED_DEF void free_##_type, _fini)

/** @cond DEV */
/**
 * Generates a reference-counting `free` function
 */
#define DEFINE_REFCNT_FREE(_type, _var)                                 \
    GENERATED_DEF void free_##_type(_type *_var)                        \
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
    GENERATED_DEF void preallocate_##_type##s(unsigned size)            \
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
#define DEFINE_ARRAY_ALLOC_COMMON(_type, _scale, _maxsize, _var,        \
                                  _init, _clone, _resize, _fini)        \
    ALLOC_COUNTERS(_type, _maxsize + 1);                                \
    static freelist_t *freelists_##_type[_maxsize];                     \
                                                                        \
    ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL            \
    static _type *alloc_##_type(unsigned n)                             \
    {                                                                   \
        _type *_var;                                                    \
        unsigned _sz = _scale##_order(n);                               \
                                                                        \
        if (_sz >= _maxsize || freelists_##_type[_sz] == NULL)          \
            _var = frlmalloc(sizeof (*_var) + _scale##_size(_sz) *      \
                             sizeof(_var->elts[0]));                    \
        else                                                            \
        {                                                               \
            _var = (_type *)freelists_##_type[_sz];                     \
            freelists_##_type[_sz] = freelists_##_type[_sz]->chain;     \
        }                                                               \
        TRACK_ALLOC_IDX(_type, _sz > _maxsize ? _maxsize : _sz);        \
        _var->nelts = n;                                                \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    GENERATED_DEF _type *new_##_type(unsigned _n)                       \
    {                                                                   \
        _type *_var = alloc_##_type(_n);                                \
        unsigned _idxvar;                                               \
                                                                        \
        for (_idxvar = 0; _idxvar < _n; _idxvar++)                      \
        {                                                               \
            _inite;                                                     \
        }                                                               \
        _initc;                                                         \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    GENERATED_DEF _type *copy_##_type(const _type *OLD(_var))           \
    {                                                                   \
        _type *NEW(_var) = alloc_##_type(OLD(_var)->nelts);             \
        unsigned _idxvar;                                               \
                                                                        \
        memcpy(NEW(_var), OLD(_var),                                    \
               sizeof(_type) +                                          \
               OLD(_var)->nelts * sizeof(OLD(_var)->elts[0]));          \
        for (_idxvar = 0; _idxvar < OLD(_var)->nelts; _idxvar++)        \
        {                                                               \
            _clonee;                                                    \
        }                                                               \
        _clonec;                                                        \
        return NEW(_var);                                               \
    }                                                                   \
                                                                        \
    ATTR_NONNULL                                                        \
    static void dispose_##_type(_type *_var)                            \
    {                                                                   \
        unsigned _sz = _scale##_order(_var->nelts);                     \
                                                                        \
        if(_sz >= _maxsize)                                             \
            free(_var);                                                 \
        else                                                            \
        {                                                               \
            ((freelist_t *)_var)->chain = freelists_##_type[_sz];       \
            freelists_##_type[_sz] = (freelist_t *)_var;                \
        }                                                               \
        TRACK_FREE_IDX(_type, _sz > _maxsize ? _maxsize : _sz);         \
    }                                                                   \
                                                                        \
    GENERATED_DEF _type *resize_##_type(_type *_var, unsigned _newn)    \
    {                                                                   \
        unsigned _idxvar;                                               \
        unsigned old_order;                                             \
        unsigned new_order;                                             \
                                                                        \
        if (_var == NULL)                                               \
            return new_##_type(_newn);                                  \
                                                                        \
        old_order = _scale##_order(_var->nelts);                        \
        new_order = _scale##_order(_newn);                              \
                                                                        \
        for (_idxvar = _newn; _idxvar < _var->nelts; _idxvar++)         \
        {                                                               \
            _finie;                                                     \
        }                                                               \
                                                                        \
        if (new_order < old_order)                                      \
        {                                                               \
            TRACK_FREE_IDX(_type,                                       \
                           old_order > _maxsize ? _maxsize : old_order); \
            TRACK_ALLOC_IDX(_type,                                      \
                            new_order > _maxsize ? _maxsize : new_order); \
        }                                                               \
        else if (new_order > old_order)                                 \
        {                                                               \
            const _type *OLD(_var) = _var;                              \
            _type *NEW(_var) = alloc_##_type(_newn);                    \
                                                                        \
            memcpy(NEW(_var), OLD(_var),                                \
                   sizeof(_type) +                                      \
                   OLD(_var)->nelts * sizeof(OLD(_var)->elts[0]));      \
            for (_idxvar = 0; _idxvar < NEW(_var)->nelts; _idxvar++)    \
            {                                                           \
                _adjuste;                                               \
            }                                                           \
            _adjustc;                                                   \
            dispose_##_type(_var);                                      \
            _var = NEW(_var);                                           \
        }                                                               \
                                                                        \
        for (_idxvar = _var->nelts; _idxvar < _newn; _idxvar++)         \
        {                                                               \
            _inite;                                                     \
        }                                                               \
        _var->nelts = _newn;                                            \
        _resizec;                                                       \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    _destructor(_type *_var)                                            \
    {                                                                   \
        unsigned _idxvar;                                               \
                                                                        \
        for (_idxvar = 0; _idxvar < _var->nelts; _idxvar++)             \
        {                                                               \
            _finie;                                                     \
        }                                                               \
        _finic;                                                         \
        dispose_##_type(_var);                                          \
    }                                                                   \
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
 * @test 
 *  Verify that the logarithmic order is correct
 * `ASSERT_UINT_EQ(log2_order(x), expected);`
 * @testvar{unsigned,x,u}  | @testvar{unsigned,expected}
 * ------------------------|---------------------------
 * `0`                     | `0`
 * `1`                     | `0`
 * `2`                     | `1`
 * `3`                     | `2`
 * `4`                     | `2`
 * `5`                     | `3`
 * `65536`                 | `16`
 */
static inline unsigned log2_order(unsigned x)
{
    return x ? ((unsigned)sizeof(unsigned) * CHAR_BIT -
                count_leading_zeroes(x - 1)) : 0;
}

/** 
 * The inverse of log2_order()
 */
#define log2_size(_x) (1u << (_x))

/**
 * Defines a linear allocation scale
 *
 * @test Background:
 * @code
 * DEFINE_LINEAR_SCALE(2);
 * DECLARE_ARRAY_TYPE(simple_linear2_array, void *ptr; unsigned tag;, unsigned);
 * DECLARE_ARRAY_ALLOCATOR(simple_linear2_array);
 * 
 * DEFINE_ARRAY_ALLOCATOR(simple_linear2_array, linear2, 4, arr, i,
 *                        {arr->tag = STATE_INITIALIZED;},
 *                        {arr->elts[i] = i; },
 *                        {NEW(arr)->tag = STATE_CLONED; },
 *                        {NEW(arr)->elts[i] = OLD(arr)->elts[i] | STATE_CLONED; },
 *                        {NEW(arr)->tag = STATE_ADJUSTED; },
 *                        {NEW(arr)->elts[i] = OLD(arr)->elts[i] | STATE_ADJUSTED; },
 *                        {arr->tag |= STATE_RESIZED; },
 *                        {arr->tag = STATE_FINALIZED; },
 *                        {arr->elts[i] = STATE_FINALIZED; });
 * @endcode
 *
 * @test 
 *  Verify that the linear scaled order is correct
 * `ASSERT_UINT_EQ(linear2_order(x), expected);`
 * @testvar{unsigned,x,u}  | @testvar{unsigned,expected}
 * ------------------------|---------------------------
 * `0`                     | `0`
 * `1`                     | `0`
 * `2`                     | `1`
 * `3`                     | `1`
 * `4`                     | `2`
 * `5`                     | `2`
 * `65536`                 | `32768`
 *
 * @test Alloc and free a scaled-linear array
 * Varying                   | From | To
 * --------------------------|------|-----
 * @testvar{unsigned,size,u} | `0`  |`4`
 * @code
 * simple_linear2_array *prev = NULL;
 * @endcode
 * - Given a fresh array:
 *   `simple_linear2_array *arr = new_simple_linear2_array(size * 2);`
 * - Verify that it is allocated
 *   `ASSERT_PTR_NEQ(arr, NULL);`
 * - Verify that it does not share memory with a lesser array:
 *   `ASSERT_PTR_NEQ(arr, prev);`
 * - Verify that it is initialized:
 *   `ASSERT_UINT_EQ(arr->tag, STATE_INITIALIZED);`
 * - Verify that the number of elements is correct:
 *   `ASSERT_UINT_EQ(arr->nelts, size * 2);`
 * - When it is freed: `free_simple_linear2_array(arr);`
 * - And when it is not so large: `if (size != 4)`
 *    + Verify that it is finalized: `ASSERT_UINT_EQ(arr->tag, STATE_FINALIZED);`
 * - Cleanup: `prev = arr;`
 *
 * @test Alloc, free and realloc a scaled-linear array
 * - Given a fresh array:
 * @code
 * simple_linear2_array *arr = new_simple_linear2_array(size * 2);
 * simple_linear2_array *arr1;
 * @endcode
 * - When it is freed: `free_simple_linear2_array(arr);`
 * - And when a new, slightly larger array, is allocated:
 *   `arr1 = new_simple_linear2_array(size * 2 + 1);`
 * - Then the memory is reused: `ASSERT_PTR_EQ(arr, arr1);`
 * - Cleanup: `free_simple_linear2_array(arr1);`
 * .
 * Varying                   | From | To
 * --------------------------|------|-----
 * @testvar{unsigned,size,u} | `0`  |`3`
 *
 * @test Resize a scaled-linear to a larger size
 * - Given an allocated scaled linear array:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * simple_linear2_array *arr = new_simple_linear2_array(sz * 2);
 * @endcode
 * - When it is reallocated to a slightly larger size:
 *   `simple_linear2_array *arr1 = resize_simple_linear2_array(arr, sz * 2 + 1);`
 * - Then the memory is reused:
 *   `ASSERT_PTR_EQ(arr, arr1);`
 * - And the number of elements is correct:
 *   `ASSERT_UINT_EQ(arr1->nelts, sz * 2 + 1);`
 * - And the extra elements are initialized:
 *   `ASSERT_UINT_EQ(arr1->elts[sz * 2], sz * 2);`
 * - When it is reallocate to a yet larger size:
 *   `arr1 = resize_simple_linear2_array(arr, (sz + 1) * 2);`
 * - Then the memory is not reused:
 *   `ASSERT_PTR_NEQ(arr, arr1);`
 * - And the number of elements is correct:
 *   `ASSERT_UINT_EQ(arr1->nelts, (sz + 1) * 2);`
 * - And the extra elements are initialized:
 *   `ASSERT_UINT_EQ(arr1->elts[sz * 2 + 1], sz * 2 + 1);` 
 * - Cleanup: `free_simple_linear2_array(arr1);`
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
                              GENERATED_DEF void free_##_type,          \
                              _finic, _finie)

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
