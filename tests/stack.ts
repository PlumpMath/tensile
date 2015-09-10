#include <stdlib.h>
#define IMPLEMENT_STACK 1
#include "stack.h"

DECLARE_STACK_TYPE(simple_stack, unsigned);
DECLARE_STACK_OPS(simple_stack, unsigned, trivial_pop, trivial_push, 4);

DEFINE_STACK_OPS(simple_stack, linear, 16, stk, i,
                 {stk->elts[i] = 0; },
                 {NEW(stk->elts[i]) = OLD(stk->elts[i]) + 1;},
                 {stk->elts[i] = 0xdeadbeef; });


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


#suite Stack
#test alloc_stack
    simple_stack *stk = new_simple_stack(8);

    ck_assert_ptr_ne(stk, NULL);
    ck_assert_uint_eq(stk->nelts, 8);
    ck_assert_uint_eq(stk->top, 0);
    ck_assert_uint_eq(stk->elts[0], 0);
    free_simple_stack(stk);
    ck_assert_uint_eq(stk->elts[0], 0);

#test push_pop
    simple_stack *stk = new_simple_stack(8);
    simple_stack *stk1 = stk;
    unsigned x;

    push_simple_stack(&stk, 1);
    ck_assert_ptr_eq(stk, stk1);
    ck_assert_uint_eq(stk->top, 1);
    x = pop_simple_stack(stk);
    ck_assert_uint_eq(stk->top, 0);
    ck_assert_uint_eq(x, 1);
    free_simple_stack(stk);

#test push_grow
    simple_stack *stk = new_simple_stack(1);
    simple_stack *stk1 = stk;
    unsigned x;

    push_simple_stack(&stk, 1);
    push_simple_stack(&stk, 2);
    ck_assert_uint_eq(stk->top, 2);
    ck_assert_ptr_ne(stk, stk1);
    ck_assert_uint_eq(stk->nelts, 6);
    ck_assert_uint_eq(stk->elts[0], 1);
    stk1 = stk;
    push_simple_stack(&stk, 3);
    ck_assert_ptr_eq(stk1, stk);
    ck_assert_uint_eq(stk->nelts, 6);
    x = pop_simple_stack(stk);
    ck_assert_uint_eq(stk->nelts, 6);
    ck_assert_uint_eq(x, 3);
    x = pop_simple_stack(stk);
    ck_assert_uint_eq(x, 2);

    free_simple_stack(stk);

#test clear_stack
    simple_stack *stk = new_simple_stack(3);

    push_simple_stack(&stk, 1);
    push_simple_stack(&stk, 2);
    push_simple_stack(&stk, 3);
    clear_simple_stack(stk);
    ck_assert_uint_eq(stk->top, 0);
    ck_assert_uint_eq(stk->nelts, 3);
    ck_assert_uint_eq(stk->elts[0], 0);
    ck_assert_uint_eq(stk->elts[1], 0);
    ck_assert_uint_eq(stk->elts[2], 0);

    free_simple_stack(stk);

#test push_pop_refcnt
    stacked_refcnt *obj = new_stacked_refcnt(0x12345);
    refcnt_stack *st = new_refcnt_stack(2);
    stacked_refcnt *obj1;

    push_refcnt_stack(&st, obj);
    ck_assert_uint_eq(obj->refcnt, 2);
    obj1 = pop_refcnt_stack(st);
    ck_assert_ptr_eq(obj, obj1);
    ck_assert_uint_eq(obj1->refcnt, 2);
    free_stacked_refcnt(obj);
    free_stacked_refcnt(obj1);

    free_refcnt_stack(st);
                        

#test stack_copy_refcnt
    refcnt_stack *st = new_refcnt_stack(2);
    refcnt_stack *st1;
    stacked_refcnt *obj = new_stacked_refcnt(0x12345);

    push_refcnt_stack(&st, obj);
    push_refcnt_stack(&st, obj);
    free_stacked_refcnt(obj);
    st1 = copy_refcnt_stack(st);
    ck_assert_ptr_ne(st1, st);
    ck_assert_uint_eq(st1->top, 2);
    ck_assert_uint_eq(obj->refcnt, 4);
    ck_assert_uint_eq(obj->value, 0x12345);
    free_refcnt_stack(st1);
    free_refcnt_stack(st);

#test ts_clear_refcnt_stack
    refcnt_stack *stk = new_refcnt_stack(3);
    stacked_refcnt *obj = new_stacked_refcnt(0x12345);

    push_refcnt_stack(&stk, obj);
    push_refcnt_stack(&stk, obj);
    push_refcnt_stack(&stk, obj);
    clear_refcnt_stack(stk);
    ck_assert_uint_eq(obj->refcnt, 1);

    free_refcnt_stack(stk);
    ck_assert_uint_eq(obj->refcnt, 1);
    free_stacked_refcnt(obj);


