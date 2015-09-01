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
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */

#include <stdlib.h>
#include "tests.h"
#define IMPLEMENT_STACK 1
#include "stack.h"

DECLARE_STACK_TYPE(simple_stack, unsigned);
DECLARE_STACK_OPS(simple_stack, unsigned, trivial_pop, trivial_push, 4);

DEFINE_STACK_OPS(simple_stack, linear, 16, stk, i,
                 {stk->elts[i] = 0; },
                 {NEW(stk->elts[i]) = OLD(stk->elts[i]) + 1;},
                 {stk->elts[i] = 0xdeadbeef; });

static void test_alloc_stack(void)
{
    simple_stack *stk = new_simple_stack(8);

    CU_ASSERT_PTR_NOT_NULL_FATAL(stk);
    CU_ASSERT_EQUAL(stk->nelts, 8);
    CU_ASSERT_EQUAL(stk->top, 0);
    CU_ASSERT_EQUAL(stk->elts[0], 0);
    free_simple_stack(stk);
    CU_ASSERT_EQUAL(stk->elts[0], 0);
}

static void test_push_pop(void)
{
    simple_stack *stk = new_simple_stack(8);
    simple_stack *stk1 = stk;
    unsigned x;

    push_simple_stack(&stk, 1);
    CU_ASSERT_PTR_EQUAL(stk, stk1);
    CU_ASSERT_EQUAL(stk->top, 1);
    x = pop_simple_stack(stk);
    CU_ASSERT_EQUAL(stk->top, 0);
    CU_ASSERT_EQUAL(x, 1);
    free_simple_stack(stk);
}

typedef struct stacked_refcnt {
    unsigned refcnt;
    unsigned value;
} stacked_refcnt;

DECLARE_REFCNT_ALLOCATOR(stacked_refcnt, ());


test_suite_descr stack_tests = {
    "stack", NULL, NULL,
    (const test_descr[]){
        TEST_DESCR(alloc_stack),
        TEST_DESCR(push_pop),
        {NULL, NULL}
    }
};
