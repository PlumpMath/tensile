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
 * @brief queue operations
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef QUEUE_H
#define QUEUE_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(IMPLEMENT_QUEUE) && !defined(IMPLEMENT_ALLOCATOR)
#define IMPLEMENT_ALLOCATOR 1
#endif

#include "allocator.h"

#define DECLARE_QUEUE_TYPE(_type, _eltype)                              \
    DECLARE_ARRAY_TYPE(_type, unsigned top; unsigned bottom;, _eltype)

#define DECLARE_QUEUE_OPS(_type, _eltype, _deqfunc)                 \
    DECLARE_ARRAY_ALLOCATOR(_type);                                 \
    extern void clear_##_type(_type *q) ATTR_NONNULL;               \
                                                                    \
    ATTR_NONNULL                                                    \
    static inline _eltype dequeue_##_type(_type *q)                 \
    {                                                               \
        assert(q->bottom != q->top);                                \
        return _deqfunc(&q->elts[q->bottom++]);                     \
    }                                                               \
                                                                    \
    ATTR_NONNULL                                                    \
    static inline _eltype dequeue_back_##_type(_type *q)            \
    {                                                               \
        assert(q->top > 0);                                         \
        return _deqfunc(&q->elts[--q->top]);                        \
    }                                                               \
                                                                    \
    ATTR_NONNULL                                                    \
    static inline unsigned _type##_size(const _type *q)             \
    {                                                               \
        assert(q->top >= q->bottom);                                \
        return q->top - q->bottom;                                  \
    }                                                               \
                                                                    \
    ATTR_NONNULL_1ST                                                \
    extern void enqueue_##_type(_type **q, _eltype item);           \
                                                                    \
    ATTR_NONNULL_1ST                                                \
    extern void enqueue_front_##_type(_type **q, _eltype item)
    
#define trivial_dequeue(_x) (*(_x))
#define trivial_enqueue(_x, _y) ((void)(*(_x) = *(_y)))

#define DECLARE_QUEUE_REFCNT_OPS(_type, _eltype)            \
    DECLARE_QUEUE_OPS(_type, _eltype *, trivial_dequeue)
    
#if IMPLEMENT_QUEUE

#define DEFINE_QUEUE_OPS(_type, _eltype, _scale, _maxsize, _var, _idxvar, \
                         _inite, _clonee, _finie, _enqfunc, _grow)      \
    DEFINE_ARRAY_ALLOCATOR(_type, _scale,                               \
                           _maxsize, _var, _idxvar,                     \
                           { _var->top = _var->bottom = 0; }, _inite,   \
                           {}, _clonee, {}, {}, {},                     \
                           { clear_##_type(_var); }, {});               \
                                                                        \
    void clear_##_type(_type *_var)                                     \
    {                                                                   \
        unsigned _idxvar;                                               \
                                                                        \
        for (_idxvar = _var->bottom; _idxvar < _var->top; _idxvar++)    \
        {                                                               \
            _finie;                                                     \
            _inite;                                                     \
        }                                                               \
        _var->bottom = _var->top = 0;                                   \
     }                                                                  \
                                                                        \
    void enqueue_##_type(_type **q, _eltype item)                       \
    {                                                                   \
        if ((*q)->top == (*q)->nelts)                                   \
        {                                                               \
            if ((*q)->bottom > 0)                                       \
            {                                                           \
                memmove((*q)->elts, &(*q)->elts[(*q)->bottom],          \
                        ((*q)->top - (*q)->bottom) * sizeof(_eltype));  \
                (*q)->top -= (*q)->bottom;                              \
                (*q)->bottom = 0;                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                *q = resize_##_type(*q, (*q)->nelts + 1 + (_grow));     \
            }                                                           \
        }                                                               \
        _enqfunc(&(*q)->elts[(*q)->top++], &item);                      \
    }                                                                   \
                                                                        \
    void enqueue_front_##_type(_type **q, _eltype item)                 \
    {                                                                   \
        if ((*q)->top == (*q)->bottom)                                  \
        {                                                               \
            (*q)->top++;                                                \
            _enqfunc(&(*q)->elts[(*q)->bottom], &item);                 \
        }                                                               \
        else                                                            \
        {                                                               \
            if ((*q)->bottom == 0)                                      \
            {                                                           \
                unsigned delta;                                         \
                                                                        \
                if ((*q)->top == (*q)->nelts)                           \
                    *q = resize_##_type(*q, (*q)->nelts + 1 + (_grow)); \
                delta = (*q)->nelts - (*q)->top;                        \
                memmove(&(*q)->elts[(*q)->bottom + delta],              \
                        &(*q)->elts[(*q)->bottom],                      \
                        ((*q)->top - (*q)->bottom) * sizeof(_eltype));  \
                (*q)->top = (*q)->nelts;                                \
                (*q)->bottom += delta;                                  \
            }                                                           \
            _enqfunc(&(*q)->elts[--(*q)->bottom], &item);               \
        }                                                               \
    }                                                                   \
    struct fake

#define DEFINE_QUEUE_REFCNT_OPS(_type, _eltype, _scale, _maxsize, _grow) \
    ATTR_NONNULL                                                        \
    static inline void do_enqueue_##_type(_eltype **dst, _eltype **src) \
    {                                                                   \
        *dst = use_##_eltype(*src);                                     \
    }                                                                   \
                                                                        \
    DEFINE_QUEUE_OPS(_type, _eltype *, _scale, _maxsize, q, i,          \
                     { q->elts[i] = NULL; },                          \
                     { NEW(q)->elts[i] =                                \
                     use_##_eltype(OLD(q)->elts[i]); },                 \
                     { free_##_eltype(q->elts[i]); },                 \
                     do_enqueue_##_type, _grow)

#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* QUEUE_H */
