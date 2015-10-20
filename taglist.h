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
 * @brief general-purpose tagged lists
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 * @test Background:
 * @code
 * #define IMPLEMENT_TAGLIST
 * @endcode
 */
#ifndef TAGLIST_H
#define TAGLIST_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#if defined(IMPLEMENT_TAGLIST) && !defined(IMPLEMENT_ALLOCATOR)
#define IMPLEMENT_ALLOCATOR 1
#endif

#include "allocator.h"

/**
 * Declare a tagged list type
 * @test Background:
 * @code
 * enum {
 *    STATE_INITIALIZED = 0xb1ff,
 *    STATE_CLONED = 0x20000000,
 *    STATE_FINALIZED = 0xdead
 * };
 * DECLARE_TAGLIST_TYPE(simple_taglist, unsigned);
 * DECLARE_TAGLIST_OPS(simple_taglist, unsigned);
 *
 * DEFINE_TAGLIST_OPS(simple_taglist, unsigned, 16, list, i,
 * {list->elts[i].value = STATE_INITIALIZED; },
 * {NEW(list)->elts[i].value = OLD(list)->elts[i].value | STATE_CLONED;},
 * {list->elts[i].value = STATE_FINALIZED; }, 4);
 * @endcode
 */
#define DECLARE_TAGLIST_TYPE(_type, _valtype)   \
    DECLARE_ARRAY_TYPE(_type,,                  \
                       struct {                 \
                           unsigned tag;        \
                           _valtype value;      \
                       })

#define NULL_TAG (0u)

/**
 * Declare operations on a tagged list:
 * - `_valtype *lookup_<_type>(_type **, unsigned, bool);`
 * - `_valtype *add_<_type>(_type **, unsigned tag);`
 * - `void delete_<_type>(_type *, unsigned, bool);`
 *
 * @test Allocate and free
 * - Given a taglist:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 16);
 * simple_taglist *list = new_simple_taglist(sz);
 * unsigned i;
 * @endcode
 * - Verify it is allocated:
 *   `ASSERT_PTR_NEQ(list, NULL);`
 * - Verify that its size is correct:
 *   `ASSERT_UINT_EQ(list->nelts, sz);`
 * - Verify that the elements are initialized:
 * @code
 * for (i = 0; i < sz; i++)
 * {
 *    ASSERT_UINT_EQ(list->elts[i].tag, NULL_TAG);
 *    ASSERT_UINT_EQ(list->elts[i].value, STATE_INITIALIZED);
 *  }
 * @endcode
 * - When it is freed:
 *  `free_simple_taglist(list);`
 * - Then the elements are finalized:
 * @code
 * for (i = 0; i < sz; i++)
 * {
 *    ASSERT_UINT_EQ(list->elts[i].tag, NULL_TAG);
 *    ASSERT_UINT_EQ(list->elts[i].value, STATE_FINALIZED);
 * }
 * @endcode
 *
 * @test Add and lookup
 * - Given a taglist:
 * @code
 * simple_taglist *list = new_simple_taglist(1);
 * simple_taglist *saved = list;
 * unsigned *found;
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval = ARBITRARY(unsigned, 1, 0xffff);
 * @endcode
 * - When a tag is added:
 *   `*add_simple_taglist(&list, thetag) = theval;`
 * - Then the taglist is not reallocated:
 *   `ASSERT_PTR_EQ(list, saved);`
 * - When the tag is looked up:
 *   `found = lookup_simple_taglist(&list, thetag, false);`
 * - Then it is found:
 *   `ASSERT_PTR_NEQ(found, NULL);`
 * - And it has the correct value:
 *   `ASSERT_UINT_EQ(*found, theval);`
 * - Cleanup: `free_simple_taglist(list);`
 *
 * @test Do Lookup
 * - Given a queue:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 3, 16);
 * unsigned thetag1 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned thetag2 = ARBITRARY(unsigned, thetag1 + 1, 0x10000);
 * unsigned thetag3 = ARBITRARY(unsigned, thetag2 + 1, 0x10001);
 * unsigned thetag4 = ARBITRARY(unsigned, thetag3 + 1, 0x10002);
 * unsigned theval1 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval2 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval3 = ARBITRARY(unsigned, 1, 0xffff);
 * simple_taglist *list = new_simple_taglist(sz);
 * unsigned *found;
 * @endcode
 * - And given several items with different tags are added:
 * @code
 * *add_simple_taglist(&list, thetag1) = theval1;
 * *add_simple_taglist(&list, thetag2) = theval2;
 * *add_simple_taglist(&list, thetag3) = theval3; 
 * @endcode
 * - When looking up each added tag:
 *   @testvar{unsigned,xtag,u} | @testvar{unsigned,xvalue,u}
 *   --------------------------|----------------------------
 *   `thetag1`                 |`theval1`
 *   `thetag2`                 |`theval2`
 *   `thetag3`                 |`theval3`
 *   `found = lookup_simple_taglist(&list, xtag, false);`
 *   + Then it is found: `ASSERT_PTR_NEQ(found, NULL);`
 *   + And the value is correct: `ASSERT_UINT_EQ(*found, xvalue);`
 * - When looking up a tag that was not added:
 *   `found = lookup_simple_taglist(&list, thetag4, false);`
 * - Then it is not found: `ASSERT_PTR_EQ(found, NULL);` 
 * - Cleanup: `free_simple_taglist(list);`
 *
 * @test Lookup Multiple
 * - Given a taglist:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 3, 16);
 * unsigned thetag1 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned thetag2 = ARBITRARY(unsigned, thetag1 + 1, 0x10000);
 * unsigned theval1 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval2 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval3 = ARBITRARY(unsigned, theval1 + 1, 0x10000);
 * simple_taglist *list = new_simple_taglist(sz);
 * unsigned *found;
 * @endcode
 * - And given several items with repeating tags are added:
 * @code
 * *add_simple_taglist(&list, thetag1) = theval1;
 * *add_simple_taglist(&list, thetag2) = theval2;
 * *add_simple_taglist(&list, thetag1) = theval3;
 * @endcode
 * - When a repeating tag is looked up:
 *   `found = lookup_simple_taglist(&list, thetag1, false);`
 * - Then it is found: `ASSERT_PTR_NEQ(found, NULL);`
 * - And the value is the first added value: `ASSERT_UINT_EQ(*found, theval1);`
 * - Cleanup: `free_simple_taglist(list);`
 *
 * @test Add and delete
 * - Given a taglist:
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * simple_taglist *list = new_simple_taglist(1);
 * unsigned *found;
 * @endcode
 * - And given an element with a new tag is added:
 * @code
 * found = lookup_simple_taglist(&list, thetag, true);
 * ASSERT_PTR_NEQ(found, NULL);
 * @endcode
 * - When the tag is deleted:
 *   `delete_simple_taglist(list, thetag, false);`
 * - And when it is looked up:
 *   `found = lookup_simple_taglist(&list, thetag, false);`
 * - Then it is not found:
 *   `ASSERT_PTR_EQ(found, NULL);`
 * - Cleanup: `free_simple_taglist(list);`
 *
 * @test Add, delete and add the same:
 * - Given a taglist:
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned thetag2 = ARBITRARY(unsigned, thetag + 1, 0x10000);
 * unsigned theval = ARBITRARY(unsigned, 1, 0xffff);
 * simple_taglist *list = new_simple_taglist(1);
 * simple_taglist *saved = list;
 * unsigned *found;
 * unsigned *added;
 * @endcode
 * - And given an element with a tag is added:
 * @code
 * found = lookup_simple_taglist(&list, thetag, true);
 * ASSERT_PTR_NEQ(found, NULL);
 * *found = theval;
 * @endcode
 * - When it is deleted:
 *   `delete_simple_taglist(list, thetag, false);`
 * - And when another tag is added:
 *   `added = add_simple_taglist(&list, thetag2);`
 * - Then the taglist is not reallocated:
 *   `ASSERT_PTR_EQ(list, saved);`
 * - And the element is at the same location as it was after the first adding
 *   `ASSERT_PTR_EQ(found, added);`
 * - And the element is re-initialized:
 *   `ASSERT_UINT_EQ(*added, STATE_INITIALIZED);`
 * - Cleanup: `free_simple_taglist(list);`
 * 
 * @test Delete a duplicate entry unhides the next added entry
 * - Given a taglist:
 * @code 
 * unsigned thetag  = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval1 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval2 = ARBITRARY(unsigned, theval1 + 1, 0x10000);
 * simple_taglist *list = new_simple_taglist(2);
 * unsigned *found;
 * @endcode
 * - And given an entry added to it:
 * @code
 * found = lookup_simple_taglist(&list, thetag, true);
 * *found = theval1;
 * @endcode
 * - And given another entry with the same tag is added to the end:
 * @code
 * found = add_simple_taglist(&list, thetag);
 * *found = theval2;    
 * @endcode
 * - When the tag is looked up:
 * `found = lookup_simple_taglist(&list, thetag, false);`
 * - Then it is found: `ASSERT_PTR_NEQ(found, NULL);`
 * - And the first value is returned: `ASSERT_UINT_EQ(*found, theval1);`
 * - When it is deleted: `delete_simple_taglist(list, thetag, false);`
 * - And when it is loooked up again:
 *   `found = lookup_simple_taglist(&list, thetag, false);`
 * - Then it is still found: `ASSERT_PTR_NEQ(found, NULL);`
 * - And the value is the second one: `ASSERT_UINT_EQ(*found, theval2);`
 * - Cleanup: `free_simple_taglist(list);`
 *
 * @test Add duplicate and Delete All
 * - Given a taglist:
 * @code
 * unsigned thetag1  = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned thetag2  = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval1 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned theval2 = ARBITRARY(unsigned, theval1 + 1, 0x10000);
 * unsigned theval3 = ARBITRARY(unsigned, theval2 + 1, 0x10001);
 * simple_taglist *list = new_simple_taglist(3);
 * unsigned *found;
 * @endcode
 * - And given an element is added to the queue:
 * @code
 * found = lookup_simple_taglist(&list, thetag1, true);
 * *found = theval1;
 * @endcode
 * - And given another element with a different tag is added:
 * @code
 * found = add_simple_taglist(&list, thetag2);
 * *found = theval2;
 * @endcode
 * - And given the third element with the same tag as the first one is
 *   added at the end:
 * @code
 * found = add_simple_taglist(&list, thetag1);
 * *found = theval3; 
 * @endcode
 * - When all the elements with tag 1 are deleted:
 *   `delete_simple_taglist(list, thetag1, true);`
 * - Then no element is found by the first tag:
 * @code
 * found = lookup_simple_taglist(&list, thetag1, false);
 * ASSERT_PTR_EQ(found, NULL);
 * @endcode
 * - And the element with the second tag is untouched:
 * @code
 * found = lookup_simple_taglist(&list, thetag2, false); 
 * ASSERT_PTR_NEQ(found, NULL);
 * ASSERT_UINT_EQ(*found, theval2);    
 * @endcode
 * - Cleanup: `free_simple_taglist(list);`
 */
#define DECLARE_TAGLIST_OPS(_type, _valtype)                            \
    DECLARE_ARRAY_ALLOCATOR(_type);                                     \
                                                                        \
    ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT                            \
    extern _valtype *lookup_##_type(_type **list, unsigned tag, bool add); \
                                                                        \
    ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL       \
    extern _valtype *add_##_type(_type **list, unsigned tag);           \
                                                                        \
    ATTR_NONNULL_1ST                                                    \
    extern void delete_##_type(_type *list, unsigned tag, bool all)

/**
 * Iterate over all elements with a given tag in the taglist
 *
 * @test Add duplicate and then iterate
 * - Given a taglist:
 * @code
 *   unsigned sz = ARBITRARY(unsigned, 1, 16);
 *   unsigned thetag1 = ARBITRARY(unsigned, 1, 0xffff);
 *   unsigned thetag2 = ARBITRARY(unsigned, thetag1 + 1, 0x10000);
 *   unsigned theval1 = ARBITRARY(unsigned, 1, 0xffff);
 *   unsigned theval2 = ARBITRARY(unsigned, theval1 + 1, 0x10000);
 *   unsigned theval3 = ARBITRARY(unsigned, theval2 + 1, 0x10001);
 *   simple_taglist *list = new_simple_taglist(sz);
 *   unsigned *found;
 *   unsigned count = 0;
 *   unsigned sum = 0;
 * @endcode
 * - And given several tags are added to it:
 * @code
 *   found = lookup_simple_taglist(&list, thetag1, true);
 *   *found = theval1;
 *   found = add_simple_taglist(&list, thetag2);
 *   *found = theval2;        
 *   found = add_simple_taglist(&list, thetag1);
 *   *found = theval3;
 * @endcode
 * - When after the list is iterared:
 * @code
 *   FOREACH_TAGGED(list, thetag1, i, 
 *                  {
 *                      count++;
 *                      sum += list->elts[i].value;
 *                  });
 * @endcode
 * - Then the count of processed elements is 2:
 *   `ASSERT_UINT_EQ(count, 2);`
 * - And the sum of processed elements is correct:
 *   `ASSERT_UINT_EQ(sum, theval1 + theval3);`
 * - Cleanup: `free_simple_taglist(list);`
 */
#define FOREACH_TAGGED(_list, _tag, _idxvar, _body)             \
    do {                                                        \
        unsigned __tag = (_tag);                                \
        unsigned _idxvar;                                       \
        for (_idxvar = 0; _idxvar < (_list)->nelts; _idxvar++)  \
        {                                                       \
            if ((_list)->elts[_idxvar].tag == __tag)            \
            {                                                   \
                _body;                                          \
            }                                                   \
        }                                                       \
    } while(0)

/**
 * Declare operations over tagged lists of refcounted objects
 * - `_valtype *get_<_type>(_type *, unsigned)`
 * - `_valtype *put_<_type>(_type **, unsigned, _valtype *)`
 * - `void replace_<_type>(_type **, unsigned, _valtype *)`
 *
 * @test Background:
 * @code
 * typedef struct tagged_refcnt {
 *     unsigned refcnt;
 *     unsigned value;
 * } tagged_refcnt;
 * 
 * DECLARE_REFCNT_ALLOCATOR(tagged_refcnt, (unsigned val));
 * DECLARE_TAGLIST_TYPE(refcnt_taglist, tagged_refcnt *);
 * DECLARE_TAGLIST_REFCNT_OPS(refcnt_taglist, tagged_refcnt);
 * 
 * DEFINE_REFCNT_ALLOCATOR(tagged_refcnt, (unsigned val),
 *                         obj, { obj->value = val; }, {},
 *                         { obj->value = 0xdeadbeef; });
 * DEFINE_TAGLIST_REFCNT_OPS(refcnt_taglist, tagged_refcnt, 16, 1);
 * @endcode
 * 
 * @test Get from empty
 * - Given an empty taglist:
 * @code 
 * unsigned sz = ARBITRARY(unsigned, 1, 16);
 * unsigned tag = ARBITRARY(unsigned, 1, UINT_MAX);
 * refcnt_taglist *list = new_refcnt_taglist(sz);
 * @endcode
 * - Verify that nothing can be retrieved from it:
 *   `ASSERT_PTR_EQ(get_refcnt_taglist(list, tag), NULL);`
 * - Cleanup: `free_refcnt_taglist(list);`
 *
 * @test Put and get
 * - Given a refcounted object:
 * @code
 * tagged_refcnt *obj = new_tagged_refcnt(ARBITRARY(unsigned, 1, 0xffff));
 * tagged_refcnt *got;
 * @endcode
 * - And given a tagged list:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 16);
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * refcnt_taglist *list = new_refcnt_taglist(sz);
 * @endcode
 * - When an object is put into an empty list:
 *   `got = put_refcnt_taglist(&list, thetag, obj);`
 * - Then the old value is null:
 *   `ASSERT_PTR_EQ(got, NULL);`
 * - And the object's refcounter is incremented:
 *   `ASSERT_UINT_EQ(obj->refcnt, 2);`
 * - When an object is got by the tag:
 *    `got = get_refcnt_taglist(list, thetag);`
 * - Then it is the same object as was put:  
 *   `ASSERT_PTR_EQ(got, obj);`
 * - And the reference counter is incremented:
 *   `ASSERT_UINT_EQ(obj->refcnt, 3);`
 * - When the obtained reference is released:
 *   `free_tagged_refcnt(got);`
 * - And when the list is freed:
 *  `free_refcnt_taglist(list);`
 * - Then the object has a reference count of 1:
 *    `ASSERT_UINT_EQ(obj->refcnt, 1);`
 * - Cleanup: `free_tagged_refcnt(obj);`
 *
 * @test Replace
 * - Given two refcounted objects:
 * @code
 * tagged_refcnt *obj = new_tagged_refcnt(ARBITRARY(unsigned, 1, 0xffff));
 * tagged_refcnt *obj1 = new_tagged_refcnt(ARBITRARY(unsigned, 1, 0xffff));
 * tagged_refcnt *got;
 * @endcode
 * - And given a tagged list:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 16);
 * refcnt_taglist *list = new_refcnt_taglist(sz);
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * @endcode
 * - When an object replaces nothing:
 *   `replace_refcnt_taglist(&list, thetag, obj);`
 * - And when another object replaces it:   
 *   `replace_refcnt_taglist(&list, thetag, obj1);`
 * - Then the first object has single reference:
 *   `ASSERT_UINT_EQ(obj->refcnt, 1);`
 * - When an object is got by the tag:
 *   `got = get_refcnt_taglist(list, thetag);`
 * - Then it is the second object:
 *   `ASSERT_PTR_EQ(got, obj1);`
 * - Cleanup:
 * @code
 * free_tagged_refcnt(got);    
 * free_tagged_refcnt(obj1);    
 * free_refcnt_taglist(list);
 * free_tagged_refcnt(obj);
 * @endcode
 *
 * @test Copy refcounted
 * - Given a refcounted object:
 * `tagged_refcnt *obj = new_tagged_refcnt(ARBITRARY(unsigned, 1, 0xffff));`
 * - And given a tagged list:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 16);
 * refcnt_taglist *list = new_refcnt_taglist(sz);
 * refcnt_taglist *copy;
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * @endcode
 * - When an object is put into the list:
 *   `replace_refcnt_taglist(&list, thetag, obj);`
 * - And when the list is copied:
 *   `copy = copy_refcnt_taglist(list);`
 * - Then the copy is successful:
 *   `ASSERT_PTR_NEQ(copy, NULL);`
 * - And the number of elements in the copy is the same as in the original:
 *   `ASSERT_UINT_EQ(copy->nelts, list->nelts);`
 * - And the object in the copy is the same as was put into the original:
 *   `ASSERT_PTR_EQ(copy->elts[0].value, obj);`
 * - And the reference counter is updated properly:
 *   `ASSERT_UINT_EQ(obj->refcnt, 3);`
 * - Cleanup:
 * @code
 * free_refcnt_taglist(list);
 * free_refcnt_taglist(copy);
 * free_tagged_refcnt(obj);
 * @endcode
 */
#define DECLARE_TAGLIST_REFCNT_OPS(_type, _valtype)                     \
    DECLARE_TAGLIST_OPS(_type, _valtype *);                             \
                                                                        \
    ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT                            \
    static inline _valtype *get_##_type(_type *list, unsigned tag)      \
    {                                                                   \
        _valtype **loc = lookup_##_type(&list, tag, false);             \
        return loc ? use_##_valtype(*loc) : NULL;                       \
    }                                                                   \
                                                                        \
                                                                        \
    ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT                            \
    static inline _valtype *put_##_type(_type **list, unsigned tag,     \
                                        _valtype *obj)                  \
    {                                                                   \
        _valtype **loc = lookup_##_type(list, tag, true);               \
        _valtype *prev;                                                 \
                                                                        \
        assert(loc != NULL);                                            \
        prev = *loc;                                                    \
        *loc = use_##_valtype(obj);                                     \
        return prev;                                                    \
    }                                                                   \
                                                                        \
    static inline void replace_##_type(_type **list, unsigned tag,      \
                                       _valtype *obj)                   \
    {                                                                   \
        free_##_valtype(put_##_type(list, tag, obj));                   \
    }                                                                   \
    struct fake

#if defined(IMPLEMENT_TAGLIST) || defined(__DOXYGEN__)

#define DEFINE_TAGLIST_OPS(_type, _valtype, _maxsize, _var, _idxvar,    \
                           _inite, _clonee, _finie, _grow)              \
    DEFINE_ARRAY_ALLOCATOR(_type, linear, _maxsize, _var, _idxvar,      \
                           {},                                          \
                           { _var->elts[_idxvar].tag = NULL_TAG; _inite; }, \
                           {}, _clonee,                                 \
                           {}, {}, {},                                  \
                           {}, _finie);                                 \
                                                                        \
    _valtype *lookup_##_type(_type **list, unsigned tag, bool add)      \
    {                                                                   \
        unsigned i;                                                     \
                                                                        \
        for (i = 0; i < (*list)->nelts; i++)                            \
        {                                                               \
            if ((*list)->elts[i].tag == tag)                            \
                return &(*list)->elts[i].value;                         \
        }                                                               \
        if (!add)                                                       \
            return NULL;                                                \
        return add_##_type(list, tag);                                  \
    }                                                                   \
                                                                        \
    _valtype *add_##_type(_type **list, unsigned tag)                   \
    {                                                                   \
        unsigned i;                                                     \
                                                                        \
        for (i = 0; i < (*list)->nelts; i++)                            \
        {                                                               \
            if ((*list)->elts[i].tag == NULL_TAG)                       \
                break;                                                  \
        }                                                               \
        if (i == (*list)->nelts)                                        \
        {                                                               \
            *list = resize_##_type(*list, (*list)->nelts + 1 + (_grow)); \
        }                                                               \
        (*list)->elts[i].tag = tag;                                     \
        return &(*list)->elts[i].value;                                 \
    }                                                                   \
                                                                        \
    void delete_##_type(_type *_var, unsigned tag, bool all)            \
    {                                                                   \
        unsigned _idxvar;                                               \
                                                                        \
        for (_idxvar = 0; _idxvar < _var->nelts; _idxvar++)             \
        {                                                               \
            if (_var->elts[_idxvar].tag == tag)                         \
            {                                                           \
                _var->elts[_idxvar].tag = 0;                            \
                _finie;                                                 \
                _inite;                                                 \
                if (!all) break;                                        \
            }                                                           \
        }                                                               \
    }                                                                   \
    struct fake

#define DEFINE_TAGLIST_REFCNT_OPS(_type, _valtype, _maxsize, _grow)     \
    DEFINE_TAGLIST_OPS(_type, _valtype *, _maxsize, list, i,            \
                       { list->elts[i].value = NULL; },                 \
                       { NEW(list)->elts[i].value =                     \
                               use_##_valtype(OLD(list)->elts[i].value); }, \
                       { free_##_valtype(list->elts[i].value); },       \
                       _grow)


#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* TAGLIST_H */
