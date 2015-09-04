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

static void test_push_grow(void)
{
    simple_stack *stk = new_simple_stack(1);
    simple_stack *stk1 = stk;
    unsigned x;

    push_simple_stack(&stk, 1);
    push_simple_stack(&stk, 2);
    CU_ASSERT_EQUAL(stk->top, 2);
    CU_ASSERT_PTR_NOT_EQUAL(stk, stk1);
    CU_ASSERT_EQUAL(stk->nelts, 6);
    CU_ASSERT_EQUAL(stk->elts[0], 1);
    stk1 = stk;
    push_simple_stack(&stk, 3);
    CU_ASSERT_PTR_EQUAL(stk1, stk);
    CU_ASSERT_EQUAL(stk->nelts, 6);
    x = pop_simple_stack(stk);
    CU_ASSERT_EQUAL(stk->nelts, 6);
    CU_ASSERT_EQUAL(x, 3);
    x = pop_simple_stack(stk);
    CU_ASSERT_EQUAL(x, 2);

    free_simple_stack(stk);
}

static void test_clear_stack(void)
{
    simple_stack *stk = new_simple_stack(3);

    push_simple_stack(&stk, 1);
    push_simple_stack(&stk, 2);
    push_simple_stack(&stk, 3);
    clear_simple_stack(stk);
    CU_ASSERT_EQUAL(stk->top, 0);
    CU_ASSERT_EQUAL(stk->nelts, 3);
    CU_ASSERT_EQUAL(stk->elts[0], 0);
    CU_ASSERT_EQUAL(stk->elts[1], 0);
    CU_ASSERT_EQUAL(stk->elts[2], 0);

    free_simple_stack(stk);
}

typedef struct stacked_refcnt {
    unsigned refcnt;
    unsigned value;
} stacked_refcnt;

DECLARE_REFCNT_ALLOCATOR(stacked_refcnt, (unsigned val));
DECLARE_STACK_TYPE(refcnt_stack, stacked_refcnt *);
DECLARE_STACK_REFCNT_OPS(refcnt_stack, stacked_refcnt, 4);

DEFINE_REFCNT_ALLOCATOR(stacked_refcnt, (unsigned val), obj,
                        { obj->value = val;},
                        { NEW(obj)->value = OLD(obj)->value + 1; },
                        { obj->value = 0; });
DEFINE_STACK_REFCNT_OPS(refcnt_stack, stacked_refcnt, linear, 8);

static void test_push_pop_refcnt(void)
{
    stacked_refcnt *obj = new_stacked_refcnt(0x12345);
    refcnt_stack *st = new_refcnt_stack(2);
    stacked_refcnt *obj1;

    push_refcnt_stack(&st, obj);
    CU_ASSERT_EQUAL(obj->refcnt, 2);
    obj1 = pop_refcnt_stack(st);
    CU_ASSERT_PTR_EQUAL(obj, obj1);
    CU_ASSERT_EQUAL(obj1->refcnt, 2);
    free_stacked_refcnt(obj);
    free_stacked_refcnt(obj1);

    free_refcnt_stack(st);
}
                        

static void test_stack_copy_refcnt(void)
{
    refcnt_stack *st = new_refcnt_stack(2);
    refcnt_stack *st1;
    stacked_refcnt *obj = new_stacked_refcnt(0x12345);

    push_refcnt_stack(&st, obj);
    push_refcnt_stack(&st, obj);
    free_stacked_refcnt(obj);
    st1 = copy_refcnt_stack(st);
    CU_ASSERT_PTR_NOT_EQUAL(st1, st);
    CU_ASSERT_EQUAL(st1->top, 2);
    CU_ASSERT_EQUAL(obj->refcnt, 4);
    CU_ASSERT_EQUAL(obj->value, 0x12345);
    free_refcnt_stack(st1);
    free_refcnt_stack(st);
}

static void test_clear_refcnt_stack(void)
{
    refcnt_stack *stk = new_refcnt_stack(3);
    stacked_refcnt *obj = new_stacked_refcnt(0x12345);

    push_refcnt_stack(&stk, obj);
    push_refcnt_stack(&stk, obj);
    push_refcnt_stack(&stk, obj);
    clear_refcnt_stack(stk);
    CU_ASSERT_EQUAL(obj->refcnt, 1);

    free_refcnt_stack(stk);
    CU_ASSERT_EQUAL(obj->refcnt, 1);
    free_stacked_refcnt(obj);
}


test_suite_descr stack_tests = {
    "stack", NULL, NULL,
    (const test_descr[]){
        TEST_DESCR(alloc_stack),
        TEST_DESCR(push_pop),
        TEST_DESCR(push_grow),
        TEST_DESCR(clear_stack),
        TEST_DESCR(push_pop_refcnt),
        TEST_DESCR(stack_copy_refcnt),
        TEST_DESCR(clear_refcnt_stack),
        {NULL, NULL}
    }
};
