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
 * @brief stack operations
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 * @test Background:
 * @code
 * #define IMPLEMENT_STACK 1
 * @endcode
 */
#ifndef STACK_H
#define STACK_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(IMPLEMENT_STACK) && !defined(IMPLEMENT_ALLOCATOR)
#define IMPLEMENT_ALLOCATOR 1
#endif

#include "allocator.h"

/**
 * Declares a stack type
 */
#define DECLARE_STACK_TYPE(_type, _eltype)              \
    DECLARE_ARRAY_TYPE(_type, unsigned top;, _eltype)

/**
 * Declare stack operations
 * - `void clear_<_type>(_type *)`
 * - `_eltype pop_<_type>(_type *)`
 * - `void push_<_type>(_type **, _eltype)`
 *
 * @test Background:
 * @code
 * enum {
 *    STATE_INITIALIZED = 0xb1ff,
 *    STATE_CLONED = 0x20000000,
 *    STATE_FINALIZED = 0xdead
 * };
 * DECLARE_STACK_TYPE(simple_stack, unsigned);
 * DECLARE_STACK_OPS(simple_stack, unsigned, trivial_pop, trivial_push, 4);
 * DEFINE_STACK_OPS(simple_stack, linear, 16, stk, i,
 * {stk->elts[i] = STATE_INITIALIZED; },
 * {NEW(stk->elts[i]) = OLD(stk->elts[i]) | STATE_CLONED;},
 * {stk->elts[i] = STATE_FINALIZED; });
 * @endcode
 *
 * @test Alloc stack
 * - Given a new stack:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 15);
 * simple_stack *stk = new_simple_stack(sz);
 * unsigned i;
 * @endcode
 * - Verify it is allocated: `ASSERT_PTR_NEQ(stk, NULL);`
 * - Verify the number of allocated elements is right:
 *   `ASSERT_UINT_EQ(stk->nelts, sz);`
 * - Verify that the top pointer is correct
 *   `ASSERT_UINT_EQ(stk->top, 0);`
 * - Verify that the elements are initialized:
 *   `for (i = 0; i < sz; i++) ASSERT_UINT_EQ(stk->elts[i], STATE_INITIALIZED);`
 * - When it is freed: `free_simple_stack(stk);`
 * - Then the elements above the top are not finalized:
 *   `for (i = 0; i < sz; i++) ASSERT_UINT_EQ(stk->elts[i], STATE_INITIALIZED);`
 *
 * @test Push and pop
 * - Given a new stack:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 2, 15);
 * simple_stack *stk = new_simple_stack(sz);
 * simple_stack *stk1 = stk;
 * unsigned x;
 * unsigned any = ARBITRARY(unsigned, 1, 0xffff);
 * @endcode
 * - When an element is pushed: `push_simple_stack(&stk, any);`
 * - Then the stack is not reallocated:
 *    `ASSERT_PTR_EQ(stk, stk1);`
 * - And the top pointer is incremented:
 *   `ASSERT_UINT_EQ(stk->top, 1);`
 * - When an element is popped back:
 *   `x = pop_simple_stack(stk);`
 * - Then the top pointer is decremented:
 *   `ASSERT_UINT_EQ(stk->top, 0);`
 * - And the popped element is the same as was pushed:
 *   `ASSERT_UINT_EQ(x, any);`
 * - Cleanup: `free_simple_stack(stk);`
 *
 * @test Push with grow
 * - Given a new stack with a single reserved element:
 * @code
 * simple_stack *stk = new_simple_stack(1);
 * simple_stack *stk1 = stk;
 * unsigned x;
 * unsigned any1 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned any2 = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned any3 = ARBITRARY(unsigned, 1, 0xffff);
 * @endcode
 * - When two elements are pushed:
 * @code
 * push_simple_stack(&stk, any1);
 * push_simple_stack(&stk, any2);
 * @endcode
 * - Then the top pointer is properly incremented:
 *   `ASSERT_UINT_EQ(stk->top, 2);`
 * - And the stack is reallocated:
 *  `ASSERT_PTR_NEQ(stk, stk1);`
 * - And the number of elements is adjusted properly:
 *  `ASSERT_UINT_EQ(stk->nelts, 6);`
 * - And the bottom element is correct:
 *   `ASSERT_UINT_EQ(stk->elts[0], any1);1
 * - When the third element is pushed:
 * @code
 * stk1 = stk;
 * push_simple_stack(&stk, any3);
 * @endcode
 * - Then the stack is not reallocated: `ASSERT_PTR_EQ(stk1, stk);`
 * - And the number of elements is the same:
 *   `ASSERT_UINT_EQ(stk->nelts, 6);`
 * - When an element is popped:
 *   `x = pop_simple_stack(stk);`
 * - Then the number of elements is still the same:
 *   `ASSERT_UINT_EQ(stk->nelts, 6);`
 * - And the popped element is the correct one:
 *   `ASSERT_UINT_EQ(x, any3);`
 * - When another element is popped:
 *   `x = pop_simple_stack(stk);`
 * - Then it is the second element:
 *   `ASSERT_UINT_EQ(x, any2);`
 * - Cleanup: `free_simple_stack(stk);`
 *
 * @test Clear stack
 * - Given a new stack
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 16);
 * unsigned n = ARBITRARY(unsigned, 1, sz);
 * simple_stack *stk = new_simple_stack(sz);
 * unsigned i;
 * @endcode
 * - When elements are pushed:
 *   `for (i = 0; i < n; i++) push_simple_stack(&stk, i);`
 * - And when a stack is cleared:
 *   `clear_simple_stack(stk);`
 * - Then the top pointer is reset:
 *   `ASSERT_UINT_EQ(stk->top, 0);`
 * - But the number of elements is intact:
 *   `ASSERT_UINT_EQ(stk->nelts, sz);`
 * - And the pushed elements are cleared:
 *   `for (i = 0; i < n; i++) ASSERT_UINT_EQ(stk->elts[i], STATE_INITIALIZED);`
 * - Cleanup: `free_simple_stack(stk);`
 */
#define DECLARE_STACK_OPS(_type, _eltype,                           \
                          _popfunc, _pushfunc, _grow)               \
    DECLARE_ARRAY_ALLOCATOR(_type);                                 \
    GENERATED_DECL void clear_##_type(_type *stk) ATTR_NONNULL;     \
                                                                    \
    ATTR_NONNULL                                                    \
    static inline _eltype pop_##_type(_type *stk)                   \
    {                                                               \
        assert(stk->top > 0);                                       \
        return _popfunc(&stk->elts[--stk->top]);                    \
    }                                                               \
                                                                    \
    ATTR_NONNULL_1ST                                                \
    static inline void push_##_type(_type **stk,                    \
                                    _eltype item)                   \
    {                                                               \
        *stk = ensure_##_type##_size(*stk, (*stk)->top + 1, _grow); \
        _pushfunc(&(*stk)->elts[(*stk)->top++], &item);             \
    }                                                               \
    struct fake

#define trivial_pop(_x) (*(_x))
#define trivial_push(_x, _y) ((void)(*(_x) = *(_y)))

/**
 * Declare operations for a stack of refcounter objects
 *
 * @test Background:
 * @code
 * typedef struct stacked_refcnt {
 *   unsigned refcnt;
 *   unsigned value;
 * } stacked_refcnt;
 *
 * DECLARE_REFCNT_ALLOCATOR(stacked_refcnt, (unsigned val));
 * DECLARE_STACK_TYPE(refcnt_stack, stacked_refcnt *);
 * DECLARE_STACK_REFCNT_OPS(refcnt_stack, stacked_refcnt, 4);
 *
 * DEFINE_REFCNT_ALLOCATOR(stacked_refcnt, (unsigned val), obj,
 *                         { obj->value = val;},
 *                         { NEW(obj)->value = OLD(obj)->value | STATE_CLONED; },
 *                         { obj->value = STATE_FINALIZED; });
 * DEFINE_STACK_REFCNT_OPS(refcnt_stack, stacked_refcnt, linear, 8);
 * @endcode
 *
 * @test Push and pop from a stack of refcounted objects 
 * - Given a stack and an object:
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned sz = ARBITRARY(unsigned, 1, 8);
 * stacked_refcnt *obj = new_stacked_refcnt(thetag);
 * refcnt_stack *st = new_refcnt_stack(sz);
 * stacked_refcnt *obj1;
 * @endcode
 * - When the object is pushed: `push_refcnt_stack(&st, obj);`
 * - Then its refcounter is incremented: `ASSERT_UINT_EQ(obj->refcnt, 2);`
 * - When the object is popped back:
 *   `obj1 = pop_refcnt_stack(st);`
 * - Then it is the same object
 *   `ASSERT_PTR_EQ(obj, obj1);`
 * - And its refcounter remains intact:
 *   `ASSERT_UINT_EQ(obj1->refcnt, 2);`
 * - Cleanup:
 * @code
 * free_stacked_refcnt(obj);
 * free_stacked_refcnt(obj1);
 * free_refcnt_stack(st);
 * @endcode
 *
 * @test Copy stack of refcounted objects
 * - Given a stack and an object:
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned sz = ARBITRARY(unsigned, 1, 8);
 * stacked_refcnt *obj = new_stacked_refcnt(thetag);
 * refcnt_stack *st = new_refcnt_stack(sz);
 * refcnt_stack *st1;
 * unsigned i;
 * @endcode
 * - When the object is pushed onto stack:
 * @code
 * push_refcnt_stack(&st, obj);
 * push_refcnt_stack(&st, obj);
 * @endcode
 * - And when the original reference is released:
 *   `free_stacked_refcnt(obj);`
 * - And when the stack is copied:
 *   `st1 = copy_refcnt_stack(st);`
 * - Then the memory of the stacks is not shared:
 *   `ASSERT_PTR_NEQ(st1, st);`
 * - And the stack pointer of the copy is correct:
 *   `ASSERT_UINT_EQ(st1->top, 2);`
 * - And the refcounter of the object is incremented properly:
 *   `ASSERT_UINT_EQ(obj->refcnt, 4);`
 * - And the object is untouched:
 *   `ASSERT_UINT_EQ(obj->value, thetag);`
 * - And the elements are not copied:
 * @code
 * for (i = 0; i < st1->top; i++) 
 *    ASSERT_PTR_EQ(st->elts[i], st1->elts[i]);
 * @endcode
 * - Cleanup:
 * @code
 * free_refcnt_stack(st1);
 * free_refcnt_stack(st);
 * @endcode
 *
 * @test Clear stack of refcounted objects
 * - Given a stack and an object:
 * @code
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xffff);
 * unsigned sz = ARBITRARY(unsigned, 1, 8);
 * stacked_refcnt *obj = new_stacked_refcnt(thetag);
 * refcnt_stack *stk = new_refcnt_stack(sz);
 * unsigned i;
 * @endcode
 * - When the element is pushed onto the stack:
 *   `for (i = 0; i < sz; i++) push_refcnt_stack(&stk, obj);`
 * - And when the stack is cleared:
 *   `clear_refcnt_stack(stk);`
 * - Then the refcounter of the object is reset:
 *   `ASSERT_UINT_EQ(obj->refcnt, 1);`
 * - When the stack is freed:
 * - `free_refcnt_stack(stk);`
 * - Then the object is not freed:
 *   `ASSERT_UINT_EQ(obj->refcnt, 1);`
 * - Cleanup: `free_stacked_refcnt(obj);`
 */
#define DECLARE_STACK_REFCNT_OPS(_type, _eltype, _grow)                 \
    ATTR_NONNULL                                                        \
    static inline void do_push_##_type(_eltype **dst, _eltype **src)    \
    {                                                                   \
        *dst = use_##_eltype(*src);                                     \
    }                                                                   \
                                                                        \
    DECLARE_STACK_OPS(_type, _eltype *, trivial_pop, do_push_##_type, _grow)
    
#if defined(IMPLEMENT_STACK) || defined(__DOXYGEN__)

#define DEFINE_STACK_OPS(_type, _scale, _maxsize, _var, _idxvar,        \
                         _inite, _clonee, _finie)                       \
    DEFINE_ARRAY_ALLOCATOR(_type, _scale, _maxsize, _var, _idxvar,      \
                           { _var->top = 0; }, _inite,                  \
                           {}, _clonee, {}, {}, {},                     \
                           { clear_##_type(_var); }, {});               \
                                                                        \
    GENERATED_DEF void clear_##_type(_type *_var)                       \
    {                                                                   \
        unsigned _idxvar;                                               \
                                                                        \
        for (_idxvar = 0; _idxvar < _var->top; _idxvar++)               \
        {                                                               \
            _finie;                                                     \
            _inite;                                                     \
        }                                                               \
        _var->top = 0;                                                  \
    }                                                                   \
    struct fake

#define DEFINE_STACK_REFCNT_OPS(_type, _eltype, _scale, _maxsize)       \
    DEFINE_STACK_OPS(_type, _scale, _maxsize, stk, i,                   \
                     { stk->elts[i] = NULL; },                          \
                     { NEW(stk)->elts[i] =                              \
                             use_##_eltype(OLD(stk)->elts[i]); },       \
                     { free_##_eltype(stk->elts[i]); })

#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* STACK_H */
