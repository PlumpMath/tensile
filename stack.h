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
 * @brief stack operations
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
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

#define DECLARE_STACK_TYPE(_type, _eltype)              \
    DECLARE_ARRAY_TYPE(_type, unsigned top; _eltype)

#define DECLARE_STACK_OPS(_type, _eltype,                           \
                          _popfunc, _pushfunc, _grow)               \
    DECLARE_ARRAY_ALLOCATOR(_type);                                 \
    extern void clear_##_type(_type *stk) ATTR_NONNULL;             \
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

#define DECLARE_STACK_REFCNT_OPS(_type, _eltype, _grow)                 \
    static inline void do_push_##type(_eltype **dst, const _eltype **src) \
    {                                                                   \
        *dst = use_##_eltype(*src);                                     \
    }                                                                   \
                                                                        \
    DECLARE_STACK_OPS(_type, _eltype *, trivial_pop, do_push_##_type, _grow)
    

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* STACK_H */
