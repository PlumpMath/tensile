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
 * @brief general-purpose tagged lists
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef TAGLIST_H
#define TAGLIST_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(IMPLEMENT_TAGLIST) && !defined(IMPLEMENT_ALLOCATOR)
#define IMPLEMENT_ALLOCATOR 1
#endif

#include "allocator.h"

#define DECLARE_TAGLIST_TYPE(_type, _valtype)   \
    DECLARE_ARRAY_TYPE(_type,,                  \
                       struct {                 \
                           unsigned tag;        \
                           _valtype value;      \
                       })

#define NULL_TAG (0u)

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
    } while(1)


#define DECLARE_TAGLIST_REFCNT_OPS(_type, _valtype)                     \
    ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT                            \
    static inline _valtype *get_##_type(_type *list, unsigned tag)      \
    {                                                                   \
        _valtype **loc = lookup_##_type(list, tag, false);              \
        return loc ? use_##_type(*loc) : NULL;                          \
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
        *loc = use_##_type(obj);                                        \
        return prev;                                                    \
    }                                                                   \
                                                                        \
    static inline void replace_##_type(_type **list, unsigned tag,      \
                                       _valtype *obj)                   \
    {                                                                   \
        free_##_valtype(put_##_type(list, tag, obj));                   \
    }                                                                   \
    struct fake

#if defined(IMPLEMENT_TAGLIST)

#define DEFINE_TAGLIST_OPS(_type, _valtype, _maxsize, _grow, _var, _idxvar, \
                           _inite, _clonee, _finie)                     \
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
            }
        }
    }
    

#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* TAGLIST_H */
