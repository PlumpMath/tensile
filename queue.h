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
 * @brief queue operations
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 * @test Background:
 * @code
 * #define IMPLEMENT_QUEUE 1
 * @endcode
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

/**
 * Declares a queue type
 */
#define DECLARE_QUEUE_TYPE(_type, _eltype)                              \
    DECLARE_ARRAY_TYPE(_type, unsigned top; unsigned bottom;, _eltype)

/** Declare queue operations
 *  - `void clear_<_type>(_type *)`
 *  - `_eltype dequeue_<_type>(_type *)`
 *  - `_eltype dequeue_back_<_type>(_type *)`
 *  - `unsigned <_type>_size(const _type *)`
 *  - `void enqueue_<_type>(_type **, _eltype)`
 *  - `void enqueue_front_<_type>(_type **, _eltype)`
 *
 * @test Background:
 * @code
 * enum {
 *    STATE_INITIALIZED = 0xb1ff,
 *    STATE_CLONED = 0x20000000,
 *    STATE_FINALIZED = 0xdead
 * };
 * DECLARE_QUEUE_TYPE(simple_queue, unsigned);
 * DECLARE_QUEUE_OPS(simple_queue, unsigned, trivial_dequeue);
 * DEFINE_QUEUE_OPS(simple_queue, unsigned, linear, 16, q, i,
 *                  {q->elts[i] = STATE_INITIALIZED; },
 *                  {NEW(q)->elts[i] = OLD(q)->elts[i] | STATE_CLONED;},
 *                  {q->elts[i] = STATE_FINALIZED; },
 *                  trivial_enqueue, 4);
 * @endcode
 * @test Alloc Queue
 * - Given a new queue:
 * @code
 *     unsigned sz = ARBITRARY(unsigned, 1, 16);
 *     simple_queue *q = new_simple_queue(sz);
 *     unsigned i;
 * @endcode
 * - Verify that it is allocated:
 *  `ASSERT_PTR_ne(q, NULL);`
 * - Verify that the number of elements is correct:
 *   `ASSERT_UINT_EQ(q->nelts, sz);`
 * - Verify that the top element is zero:
 *   `ASSERT_UINT_EQ(q->top, 0);`
 * - Verify that the bottom element is zero:
 *   `ASSERT_UINT_EQ(q->bottom, 0);`
 * - Verify that elements are initialized:
 *   `for (i = 0; i < sz; i++) ASSERT_UINT_EQ(q->elts[0], STATE_INITIALIZED);`
 * - When it is freed: `free_simple_queue(q);`
 * - Then the elements are not finalized:
 *   `for (i = 0; i < sz; i++) ASSERT_UINT_EQ(q->elts[0], STATE_INITIALIZED);`
 * @test Enqueue and dequeue
 * - Given a queue:
 * @code
 *     unsigned sz = ARBITRARY(unsigned, 1, 16);
 *     simple_queue *q = new_simple_queue(8);
 *     simple_queue *q1 = q;
 *     unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 *     unsigned x;
 * @endcode
 * - When an element is enqueued:
 *   `enqueue_simple_queue(&q, thetag);`
 * - Then the queue is not reallocated:
 *   `ASSERT_PTR_EQ(q, q1);`
 * - And the top pointer is incremented:
 *   `ASSERT_UINT_EQ(q->top, 1);`
 * - And the bottom pointer is intact:
 *   `ASSERT_UINT_EQ(q->bottom, 0);`
 * - When the element is dequeued:
 *   `x = dequeue_simple_queue(q);`
 * - Then the top pointer is intact:
 *   `ASSERT_UINT_EQ(q->top, 1);`
 * - And the bottom pointer is incremented:
 *   `ASSERT_UINT_EQ(q->bottom, 1);`
 * - And the dequeued element is the same as one enqueued:
 *   `ASSERT_UINT_EQ(x, thetag);`
 * - Cleanup: `free_simple_queue(q);`
 * @test Background:
 * - Given a queue:
 * @code
 *     unsigned sz = ARBITRARY(unsigned, 2, 16);
 *     simple_queue *q = new_simple_queue(sz);
 *     unsigned thetag1 = ARBITRARY(unsigned, 1, 0xffff);
 *     unsigned thetag2 = ARBITRARY(unsigned, 1, 0xffff);
 * @endcode
 * @test Enqueue two and dequeue
 * - Given a queue:
 * @code
 *     simple_queue *q1 = q;
 *     unsigned x;
 * @endcode
 * - When two elements are enqueued:
 * @code
 *     enqueue_simple_queue(&q, thetag1);
 *     enqueue_simple_queue(&q, thetag2);    
 * @endcode
 * - Then the queue is not reallocated:
 *   `ASSERT_PTR_eq(q, q1);`
 * - And the top pointer is incremented:
 *   `ASSERT_UINT_EQ(q->top, 2);`
 * - But the bottom pointer is intact:
 *   `ASSERT_UINT_EQ(q->bottom, 0);`
 * - When an element is dequeued:
 *   `x = dequeue_simple_queue(q);`
 * - Then the top pointer is intact:
 *   `ASSERT_UINT_EQ(q->top, 2);`
 * - And the bottom pointer is incremented:
 *   `ASSERT_UINT_EQ(q->bottom, 1);`
 * - And the dequeued element is the first one that was enqueued:
 *   `ASSERT_UINT_EQ(x, thetag1);`
 * - Cleanup: free_simple_queue(q);
 * @test Enqueue and dequeue from the back
 * - Given a queue:
 * @code
 *     unsigned sz = ARBITRARY(unsigned, 2, 16);
 *     simple_queue *q = new_simple_queue(sz);
 *     unsigned thetag1 = ARBITRARY(unsigned, 1, 0xffff);
 *     unsigned thetag2 = ARBITRARY(unsigned, 1, 0xffff);
 *     unsigned x;
 * @endcode
 * - When two elements are enqueued:
 * @code
 *     enqueue_simple_queue(&q, thetag1);
 *     enqueue_simple_queue(&q, thetag2);
 * @endcode
 * - And when an element is dequeued from the back:
 *   `x = dequeue_back_simple_queue(q);`
 * - Then the top pointer is decremented:
 *   `ASSERT_UINT_EQ(q->top, 1);`
 * - And the bottom pointer is intact:
 *   `ASSERT_UINT_EQ(q->bottom, 0);`
 * - And the dequeued element is the last one that was enqueued:
 *   `ASSERT_UINT_EQ(x, thetag2);`
 * - Cleanup: `free_simple_queue(q);`
 * @test Enqueue at the front
 * - Given a queue:
 * @code
 *     unsigned sz = ARBITRARY(unsigned, 2, 16);
 *     simple_queue *q = new_simple_queue(sz);
 *     unsigned thetag1 = ARBITRARY(unsigned, 1, 0xffff);
 *     unsigned thetag2 = ARBITRARY(unsigned, 1, 0xffff);
 *     unsigned x;
 * @endcode
 * - When an element is enqueued at the front:
 *   `enqueue_front_simple_queue(&q, thetag1);`
 * - Then the top pointer is incremented:
 *   `ASSERT_UINT_EQ(q->top, 1);`
 * - And the bottom pointer is intact:
 *   `ASSERT_UINT_EQ(q->bottom, 0);`
 * - When the second element is enqueued:
 *   `enqueue_front_simple_queue(&q, thetag2);`
 * - Then the top pointer is at the top of array:
 *   `ASSERT_UINT_EQ(q->top, sz);`
 * - And the bottom pointer is 2 elements lower:
 *   `ASSERT_UINT_EQ(q->bottom, sz - 2);`
 * - Cleanup: `free_simple_queue(q);`
 * @test Enqueue, dequeue and enqueue at the front 
 *     simple_queue *q = new_simple_queue(8);
 *     unsigned x;
 * 
 *     enqueue_simple_queue(&q, 1);
 *     enqueue_simple_queue(&q, 2);
 *     dequeue_simple_queue(q);
 *     enqueue_front_simple_queue(&q, 3);
 *     ASSERT_UINT_EQ(q->bottom, 0);
 *     ASSERT_UINT_EQ(q->top, 2);
 *     x = dequeue_simple_queue(q);
 *     ASSERT_UINT_EQ(x, 3);
 *     free_simple_queue(q);
 * 
 * 
 * @test enqueue_grow
 *     simple_queue *q = new_simple_queue(1);
 *     simple_queue *q1 = q;
 *     unsigned x;
 * 
 *     enqueue_simple_queue(&q, 1);
 *     enqueue_simple_queue(&q, 2);
 *     ASSERT_UINT_EQ(q->top, 2);
 *     ASSERT_PTR_ne(q, q1);
 *     ASSERT_UINT_EQ(q->nelts, 6);
 *     ASSERT_UINT_EQ(q->elts[0], 1);
 *     q1 = q;
 *     enqueue_simple_queue(&q, 3);
 *     ASSERT_PTR_eq(q1, q);
 *     ASSERT_UINT_EQ(q->nelts, 6);
 *     x = dequeue_simple_queue(q);
 *     ASSERT_UINT_EQ(q->nelts, 6);
 *     ASSERT_UINT_EQ(x, 1);
 *     x = dequeue_simple_queue(q);
 *     ASSERT_UINT_EQ(x, 2);
 *     x = dequeue_simple_queue(q);
 *     ASSERT_UINT_EQ(x, 3);
 * 
 *     free_simple_queue(q);
 * 
 * @test enqueue_front_grow
 *     simple_queue *q = new_simple_queue(1);
 *     simple_queue *q1 = q;
 * 
 *     enqueue_front_simple_queue(&q, 1);
 *     enqueue_front_simple_queue(&q, 2);
 *     ASSERT_PTR_ne(q, q1);
 *     ASSERT_UINT_EQ(q->top, 6);
 *     ASSERT_UINT_EQ(q->bottom, 4);    
 *     ASSERT_UINT_EQ(q->nelts, 6);
 *     ASSERT_UINT_EQ(q->elts[4], 2);
 *     ASSERT_UINT_EQ(q->elts[5], 1);
 * 
 *     free_simple_queue(q);
 * 
 * 
 * @test clear_queue
 *     simple_queue *q = new_simple_queue(3);
 * 
 *     enqueue_simple_queue(&q, 1);
 *     enqueue_simple_queue(&q, 2);
 *     enqueue_simple_queue(&q, 3);
 *     clear_simple_queue(q);
 *     ASSERT_UINT_EQ(q->top, 0);
 *     ASSERT_UINT_EQ(q->bottom, 0);    
 *     ASSERT_UINT_EQ(q->nelts, 3);
 *     ASSERT_UINT_EQ(q->elts[0], 0);
 *     ASSERT_UINT_EQ(q->elts[1], 0);
 *     ASSERT_UINT_EQ(q->elts[2], 0);
 * 
 *     free_simple_queue(q);
 *  */
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
