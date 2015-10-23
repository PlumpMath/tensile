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
 * sets of reference-counted objects
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 *
 * @test Background:
 * @code
 * #define IMPLEMENT_REFSET 1
 * @endcode
 */
#ifndef REFSET_H
#define REFSET_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(IMPLEMENT_REFSET) && !defined(IMPLEMENT_ALLOCATOR)
#define IMPLEMENT_ALLOCATOR 1
#endif

#include <stdbool.h>
#include <stdarg.h>
#include "allocator.h"


/** Declares a refset type
 */
#define DECLARE_REFSET_TYPE(_type, _reftype) \
    DECLARE_ARRAY_TYPE(_type,, _reftype *)

/** Declares refset operations:
 *  - `bool is_in_<_type>(const _type *, const _reftype *)`
 *  - `void include_<_type>(_type **, _reftype *)`
 *  - `bool exclude_<_type>(_type *, const _reftype *)`
 *  - `unsigned count_<_type>(const _type *)`
 *  - `bool foreach_<_type>(const _type *, bool (*func)(_reftype, void *), void *)`
 *  - `void clear_#_type(_type *)`
 *
 * @test Background:
 * @code
 * typedef struct simple_ref {
 *     unsigned refcnt;
 *     unsigned value;
 * } simple_ref;
 * 
 * DECLARE_REFCNT_ALLOCATOR(simple_ref, (unsigned val));
 * DECLARE_REFSET_TYPE(simple_refset, simple_ref);
 * DECLARE_REFSET_OPS(simple_refset, simple_ref);
 * DEFINE_REFCNT_ALLOCATOR(simple_ref, (unsigned val),
 *                         obj, { obj->value = val; },
 *                         {}, {});
 * DEFINE_REFSET_OPS(simple_refset, simple_ref, 16, 4);
 * @endcode
 *
 * @test Background:
 * - Given an empty refset:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 0, 16);
 * simple_refset *refset = new_simple_refset(sz);
 * @endcode
 * - And given any ref object:
 *   `simple_ref *obj = new_simple_ref(ARBITRARY(unsigned, 0, 0xffff));`
 *
 * @test Not in empty refset
 * - Verify that the set does not contain the object:
 *   `ASSERT(!is_in_simple_refset(refset, obj));`
 * - Cleanup:
 * @code
 * free_simple_refset(refset);
 * free_simple_ref(obj);
 * @endcode
 *
 * @test Include and check
 * - When the object is included:
 *   `include_simple_refset(&refset, obj);`
 * - Then it is in the set:
 *   `ASSERT(is_in_simple_refset(refset, obj));`
 * - And the reference counter is incremented:
 *   `ASSERT_UINT_EQ(obj->refcnt, 2);`
 * - Cleanup:
 * @code
 * free_simple_refset(refset);
 * free_simple_ref(obj);
 * @endcode
 *
 * @test Include multiple and check
 * - And given another ref object:
 *   `simple_ref *obj2 = new_simple_ref(ARBITRARY(unsigned, 0, 0xffff));`
 * - When the two objects are added:
 *   `include_simple_refset_list(&refset, obj, obj2, NULL);`
 * - Then they both are in the set:
 *   `ASSERT(is_in_simple_refset(refset, obj));`
 *   `ASSERT(is_in_simple_refset(refset,obj2));`
 * - And they both have their reference counters incremented:
 *   `ASSERT_UINT_EQ(obj->refcnt, 2);`
 *   `ASSERT_UINT_EQ(obj2->refcnt, 2);`
 * - And the estimated size of refset is correct:
 *   `ASSERT_UINT_EQ(count_simple_refset(refset), 2);`
 * - Cleanup:
 * @code
 * free_simple_refset(refset);
 * free_simple_ref(obj);
 * free_simple_ref(obj2);
 * @endcode
 *
 * @test Equality is Identity
 * - And given another ref object which is the copy of the first:
 *   `simple_ref *obj2 = copy_simple_ref(obj);` 
 * - When the first object is included:
 *   `include_simple_refset(&refset, obj);`
 * - Then the second one is not in the set:
 *   `ASSERT(!is_in_simple_refset(refset, obj2));`
 * - Cleanup:
 * @code
 * free_simple_refset(refset);
 * free_simple_ref(obj);
 * free_simple_ref(obj2);
 * @endcode
 *
 * @test Exclude from empty
 * - Verify that excluding the object has no effect:
 *   `ASSERT(!exclude_simple_refset(refset, obj));`
 * - Cleanup:
 * @code
 * free_simple_refset(refset);
 * free_simple_ref(obj);
 * @endcode
 *
 * @test Include and exclude
 * - When the object is included:
 *   `include_simple_refset(&refset, obj);`
 * - Then it can be excluded:
 *   `ASSERT(exclude_simple_refset(refset, obj));`
 * - And it is not in the set afterwards:
 *   `ASSERT(!is_in_simple_refset(refset, obj));`
 * - And it has single reference:
 *   `ASSERT_UINT_EQ(obj->refcnt, 1);`
 * - And the second exclude has no effect:
 *   `ASSERT(!exclude_simple_refset(refset, obj));`
 * - And the object still has a reference:
 *   `ASSERT_UINT_EQ(obj->refcnt, 1);`
 * - Cleanup:
 * @code
 * free_simple_refset(refset);
 * free_simple_ref(obj);
 * @endcode
 *
 * @test Include and clear
 * - Given the object is included:
 *   `include_simple_refset(&refset, obj);`
 * - When the refset is cleared:
 *   `clear_simple_refset(refset);`
 * - Then the object is no more in the refset:
 *   `ASSERT(!is_in_simple_refset(refset, obj));`
 * - And the size of the refset is zero:
 *   `ASSERT_UINT_EQ(count_simple_refset(refset), 0);`
 * - And the object has a single reference:
 *   `ASSERT_UINT_EQ(obj->refcnt, 1);`
 * - Cleanup:
 * @code
 * free_simple_ref(obj);
 * free_simple_refset(refset);
 * @endcode
 *
 * @test Include multiple and clear
 * - Given another object:
 *   `simple_ref *obj2 = new_simple_ref(ARBITRARY(unsigned, 0, 0xffff));`
 * - And given they both are included:
 *   `include_simple_refset_list(&refset, obj, obj2, NULL);`
 * - When the refset is cleared:
 *   `clear_simple_refset(refset);`
 * - Then the size of the refset is zero:
 *   `ASSERT_UINT_EQ(count_simple_refset(refset), 0);`
 * - And the objects has a single reference:
 *   `ASSERT_UINT_EQ(obj->refcnt, 1);`
 *   `ASSERT_UINT_EQ(obj2->refcnt, 1);`
 * - Cleanup:
 * @code
 * free_simple_ref(obj2);
 * free_simple_ref(obj);
 * free_simple_refset(refset);
 * @endcode
 * 
 * @test Include multiple and iterate
 * - Background:
 * @code
 * static bool test_iter(simple_ref *r, void *arg)
 * {
 *    return r != arg;
 * }
 * @endcode
 * - 
 *
 * @test Background:
 * - Given an empty refset with a single reserved element:
 * @code
 * simple_refset *refset = new_simple_refset(1);
 * simple_refset *saved = refset;
 * @endcode
 *
 * @test Include grow
 * - Given three objects:
 * @code
 * simple_ref *obj = new_simple_ref(ARBITRARY(unsigned, 0, 0xffff));
 * simple_ref *obj2 = new_simple_ref(ARBITRARY(unsigned, 0, 0xffff));
 * simple_ref *obj3 = new_simple_ref(ARBITRARY(unsigned, 0, 0xffff));
 * @endcode
 * - When the first object is added:
 *   `include_simple_refset(&refset, obj);`
 * - Then the refset is not reallocated:
 *   `ASSERT_PTR_EQ(refset, saved);`
 * - When the second object is added:
 *  `include_simple_refset(&refset, obj2);`
 * - Then the refset is reallocated:
 *   `ASSERT_PTR_NEQ(refset, saved);`
 * - When the third object is added:
 *  `saved = refset; include_simple_refset(&refset, obj3);`
 * - Then the refset is not reallocated:
 *   `ASSERT_PTR_EQ(refset, saved);`
 * - Cleanup:
 * @code
 * free_simple_ref(obj);
 * free_simple_ref(obj2);
 * free_simple_ref(obj3);
 * free_simple_refset(refset);
 * @endcode
 *
 * @test Include same No grow
 * - Given an objects:
 * @code
 * simple_ref *obj = new_simple_ref(ARBITRARY(unsigned, 0, 0xffff));
 * @endcode
 * - When the same object is included twice:
 * @code
 * include_simple_refset(&refset, obj);
 * include_simple_refset(&refset, obj);
 * @endcode
 * - Then the refset is not reallocated:
 *   `ASSERT_PTR_EQ(refset, saved);`
 * - Cleanup:
 * @code
 * free_simple_ref(obj);
 * free_simple_refset(refset);
 * @endcode
 */
#define DECLARE_REFSET_OPS(_type, _reftype)                             \
    DECLARE_ARRAY_ALLOCATOR(_type);                                     \
                                                                        \
    ATTR_WARN_UNUSED_RESULT ATTR_PURE                                   \
    extern bool is_in_##_type(const _type *set, const _reftype *obj);   \
    extern void include_##_type(_type **set, _reftype *obj);            \
                                                                        \
    ATTR_SENTINEL                                                       \
    extern void include_##_type##_list(_type **set, ...);               \
    extern bool exclude_##_type(_type *set, const _reftype *obj);       \
                                                                        \
    ATTR_WARN_UNUSED_RESULT ATTR_PURE                                   \
    extern unsigned count_##_type(const _type *set);                    \
                                                                        \
    ATTR_NONNULL_ARGS((2))                                              \
    extern void filter_##_type(_type *set,                              \
                               bool (*func)(const _reftype *, void *),  \
                               void *extra);                            \
                                                                        \
    ATTR_NONNULL_ARGS((2))                                              \
    extern bool foreach_##_type(const _type *set,                       \
                                bool (*func)(_reftype *, void *),       \
                                void *extra);                           \
    extern void clear_##_type(_type *set)
        
#if defined(IMPLEMENT_REFSET) || defined(__DOXYGEN__)

#define DEFINE_REFSET_OPS(_type, _reftype, _maxsize, _grow)             \
    DEFINE_ARRAY_ALLOCATOR(_type, linear, _maxsize, obj, idx,           \
                           {},                                          \
                           { obj->elts[idx] = NULL; },                  \
                           {},                                          \
                           { NEW(obj)->elts[idx] =                      \
                               use_##_reftype(OLD(obj)->elts[idx]); },  \
                           {}, {}, {},                                  \
                           {},                                          \
                           { free_##_reftype(obj->elts[idx]);});        \
                                                                        \
    bool is_in_##_type(const _type *set, const _reftype *obj)           \
    {                                                                   \
        unsigned i;                                                     \
                                                                        \
        if (set == NULL || obj == NULL)                                 \
            return false;                                               \
        for (i = 0; i < set->nelts; i++)                                \
        {                                                               \
            if (set->elts[i] == obj)                                    \
                return true;                                            \
        }                                                               \
        return false;                                                   \
    }                                                                   \
                                                                        \
    void include_##_type(_type **set, _reftype *obj)                    \
    {                                                                   \
        unsigned i;                                                     \
        if (is_in_##_type(*set, obj) || obj == NULL)                    \
            return;                                                     \
        if (*set == NULL)                                               \
        {                                                               \
            (*set) = new_##_type(1 + (_grow));                          \
            (*set)->elts[0] = use_##_reftype(obj);                      \
            return;                                                     \
        }                                                               \
                                                                        \
        for (i = 0; i < (*set)->nelts; i++)                             \
        {                                                               \
            if ((*set)->elts[i] == NULL)                                \
            {                                                           \
                (*set)->elts[i] = use_##_reftype(obj);                  \
                return;                                                 \
            }                                                           \
        }                                                               \
        *set = resize_##_type(*set, (*set)->nelts + 1 + (_grow));       \
        (*set)->elts[i] = use_##_reftype(obj);                          \
    }                                                                   \
                                                                        \
    void include_##_type##_list(_type **set, ...)                       \
    {                                                                   \
        va_list args;                                                   \
        _reftype *next;                                                 \
                                                                        \
        va_start(args, set);                                            \
        while ((next = va_arg(args, _reftype *)) != NULL)               \
        {                                                               \
            include_##_type(set, next);                                 \
        }                                                               \
    }                                                                   \
                                                                        \
    bool exclude_##_type(_type *set, const _reftype *obj)               \
    {                                                                   \
        unsigned i;                                                     \
                                                                        \
        if (set == NULL || obj == NULL)                                 \
            return false;                                               \
                                                                        \
        for (i = 0; i < set->nelts; i++)                                \
        {                                                               \
            if (set->elts[i] == obj)                                    \
            {                                                           \
                free_##_reftype(set->elts[i]);                          \
                set->elts[i] = NULL;                                    \
                return true;                                            \
            }                                                           \
        }                                                               \
        return false;                                                   \
    }                                                                   \
                                                                        \
    unsigned count_##_type(const _type *set)                            \
    {                                                                   \
        unsigned i;                                                     \
        unsigned result = 0;                                            \
                                                                        \
        if (set == NULL)                                                \
            return 0;                                                   \
                                                                        \
        for (i = 0; i < set->nelts; i++)                                \
        {                                                               \
            if (set->elts[i] != NULL)                                   \
                result++;                                               \
        }                                                               \
        return result;                                                  \
    }                                                                   \
                                                                        \
    void filter_##_type(_type *set,                                     \
                        bool (*func)(const _reftype *, void *),         \
                        void *extra)                                    \
    {                                                                   \
        unsigned i;                                                     \
                                                                        \
        if (set == NULL)                                                \
            return;                                                \
        for (i = 0; i < set->nelts; i++)                                \
        {                                                               \
            if (set->elts[i] != NULL)                                   \
            {                                                           \
                if (!func(set->elts[i], extra))                         \
                {                                                       \
                    free_##_reftype(set->elts[i]);                      \
                    set->elts[i] = NULL;                                \
                }                                                       \
            }                                                           \
        }                                                               \
    }                                                                   \
                                                                        \
    bool foreach_##_type(const _type *set,                              \
                        bool (*func)(_reftype *, void *),               \
                        void *extra)                                    \
    {                                                                   \
        unsigned i;                                                     \
                                                                        \
        if (set == NULL)                                                \
            return true;                                                \
        for (i = 0; i < set->nelts; i++)                                \
        {                                                               \
            if (set->elts[i] != NULL)                                   \
            {                                                           \
                if (!func(set->elts[i], extra))                         \
                    return false;                                       \
            }                                                           \
        }                                                               \
        return true;                                                    \
    }                                                                   \
                                                                        \
    void clear_##_type(_type *set)                                      \
    {                                                                   \
        unsigned i;                                                     \
                                                                        \
        if (set == NULL)                                                \
            return;                                                     \
        for (i = 0; i < set->nelts; i++)                                \
        {                                                               \
            if (set->elts[i] != NULL)                                   \
            {                                                           \
                free_##_reftype(set->elts[i]);                          \
                set->elts[i] = NULL;                                    \
            }                                                           \
        }                                                               \
    }                                                                   \
    struct fake
    
    
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* REFSET_H */
