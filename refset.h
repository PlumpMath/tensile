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
 * DEFINE_REFSET_OPS(simple_refset, simple_ref, 16, 4);
 * @endcode
 *
 * @test Not in empty refset
 * - Given an empty refset:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 0, 16);
 * simple_refset *refset = new_simple_refset(sz);
 * @endcode
 * - And given any ref object:
 *   `simple_ref *obj = new_simple_ref(ARBITRARY(unsigned, 0, 0xffff));`
 * - Verify that the set does not contain the object:
 *   `ASSERT(!is_in_simple_refset(refset, obj));`
 * - Cleanup:
 * @code
 * free_simple_refset(refset);
 * free_simple_ref(obj);
 * @endcode
 */
#define DECLARE_REFSET_OPS(_type, _reftype)                             \
    DECLARE_ARRAY_ALLOCATOR(_type);                                     \
                                                                        \
    ATTR_WARN_UNUSED_RESULT ATTR_PURE                                   \
    extern bool is_in_##_type(const _type *set, const _reftype *obj);   \
    extern void include_##_type(_type **set, _reftype *obj);            \
    extern bool exclude_##_type(_type *set, const _reftype *obj);       \
    extern unsigned count_##_type(const _type *set);                    \
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
        if (is_in_##type(*set, obj) || obj == NULL)                     \
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
        *set = resize_##_type((*set)->nelts + 1 + (_grow));             \
        (*set)->elts[i] = use_##_reftype(obj);                          \
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
