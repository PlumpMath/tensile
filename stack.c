/*!= Generic stack routines
 * Copyright (c) 2015-2016  Artem V. Andreev
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

/* HEADER */
#include "compiler.h"
/* END */

/* parameter */
#define STACK_TYPE /* required */
/* parameter */
#define STACK_NAME STACK_TYPE
/* parameter */
#define ELEMENT_TYPE /* required */
/* parameter */
#define ALLOC_NAME ELEMENT_TYPE
/* parameter */
#define ELTS_FIELD data
/* parameter */
#define SIZE_FIELD size
/* parameter */
#define TOP_FIELD top
/* parameter */
#define GROW_RESERVE 0

/* parameter */
#define POP_CLEANUP_CODE(_obj) {}

/* TESTS */
#include "assertions.h"
#define TESTSUITE "Stacks"

typedef struct simple_stack {
    size_t size;
    size_t top;
    unsigned *data;
} simple_stack;

#define STACK_TAG STATIC_ARBITRARY(16)

/* instantiate */
#define STACK_TYPE simple_stack
#define ELEMENT_TYPE unsigned
#define ALLOC_TYPE ELEMENT_TYPE
#define POP_CLEANUP_CODE(_obj) (*(_obj) = STACK_TAG)
#define ALLOC_CONSTRUCTOR_CODE(_obj) (*(_obj) = STACK_TAG)
#define MAX_BUCKET (TESTVAL_SMALL_UINT_MAX)
#include "array_api.h"
#include "array_impl.c"
#include "stack_api.h"
#include "stack_impl.c"
/* end */

/* END */


/* local */
#define init_stack_TYPE QNAME(init_stack, STACK_NAME)

/* public */
static inline arguments(not_null)
void init_stack_TYPE(STACK_TYPE *stack)
{
    assert(stack != NULL);
    stack->ELTS_FIELD = NULL;
    stack->SIZE_FIELD = 0;
    stack->TOP_FIELD = 0;
}


/* local */
#define push_TYPE QNAME(push, STACK_NAME)

/* public */
static inline returns(not_null) arguments(not_null) returns(important)
ELEMENT_TYPE *push_TYPE(STACK_TYPE *stack, size_t n)
{
    ELEMENT_TYPE *result;
    
    assert(stack != NULL);
    assert(n > 0);
    QNAME(QNAME(ensure, ALLOC_NAME), size)(&stack->SIZE_FIELD,
                                           &stack->ELTS_FIELD,
                                           stack->TOP_FIELD + n,
                                           GROW_RESERVE);
    result = stack->ELTS_FIELD + stack->TOP_FIELD;
    stack->TOP_FIELD += n;
    return result;
}

/*! Test: Push & push
 */
static void test_push(testval_small_uint_t n)
{
    simple_stack stk;
    unsigned *topmost;

    init_stack_simple_stack(&stk);
    ASSERT_NULL(stk.data);
    ASSERT_EQ(unsigned, stk.size, 0);
    ASSERT_EQ(unsigned, stk.top, 0);

    topmost = push_simple_stack(&stk, n + 1);
    ASSERT_NOT_NULL(topmost);
    ASSERT_EQ(ptr, topmost, stk.data);
    ASSERT_EQ(unsigned, stk.size, stk.top);
    ASSERT_EQ(unsigned, stk.top, n + 1);

    topmost = push_simple_stack(&stk, n + 1);
    ASSERT_NOT_NULL(topmost);
    ASSERT_EQ(ptr, topmost, stk.data + n + 1);
    ASSERT_EQ(unsigned, stk.size, stk.top);
    ASSERT_EQ(unsigned, stk.top, 2 * (n + 1));

    clear_simple_stack(&stk);
    ASSERT_NULL(stk.data);
    ASSERT_EQ(unsigned, stk.size, 0);
    ASSERT_EQ(unsigned, stk.top, 0);
}

/* local */
#define pop_TYPE QNAME(pop, STACK_NAME)

/* public */
static inline arguments(not_null) returns(important)
ELEMENT_TYPE pop_TYPE(STACK_TYPE *stack)
{
    ELEMENT_TYPE result;
    
    assert(stack != NULL);
    assert(stack->TOP_FIELD > 0);
    
    stack->TOP_FIELD--;
    result = stack->ELTS_FIELD[stack->TOP_FIELD];
    POP_CLEANUP_CODE((&stack->ELTS_FIELD[stack->TOP_FIELD]));
    return result;
}

/*! Test: Push & pop
 */
static void test_push_pop(testval_small_uint_t n, testval_tag_t tag)
{
    simple_stack stk;
    unsigned result;
    unused unsigned *topmost = NULL;
    
    init_stack_simple_stack(&stk);
    if (n > 0)
        topmost = push_simple_stack(&stk, n);
    *push_simple_stack(&stk, 1) = tag;
    result = pop_simple_stack(&stk);
    ASSERT_EQ(unsigned, result, tag);
    ASSERT_NOT_NULL(stk.data);
    ASSERT_EQ(unsigned, stk.top, n);
    ASSERT_EQ(unsigned, stk.size, n + 1);
    ASSERT_EQ(unsigned, stk.data[stk.top], STACK_TAG);
    clear_simple_stack(&stk);
}

/* local */
#define clear_TYPE QNAME(clear, STACK_NAME)

/* public */
static inline arguments(not_null)
void clear_TYPE(STACK_TYPE *stack)
{
    assert(stack != NULL);
    QNAME(free, ALLOC_NAME)(stack->SIZE_FIELD, stack->ELTS_FIELD);
    init_stack_TYPE(stack);
}


