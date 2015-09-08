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
#define IMPLEMENT_QUEUE 1
#include "queue.h"

DECLARE_QUEUE_TYPE(simple_queue, unsigned);
DECLARE_QUEUE_OPS(simple_queue, unsigned, trivial_dequeue);

DEFINE_QUEUE_OPS(simple_queue, unsigned, linear, 16, q, i,
                 {q->elts[i] = 0; },
                 {NEW(q)->elts[i] = OLD(q)->elts[i] + 1;},
                 {q->elts[i] = 0xdeadbeef; },
                 trivial_enqueue, 4);

static void test_alloc_queue(void)
{
    simple_queue *q = new_simple_queue(8);

    CU_ASSERT_PTR_NOT_NULL_FATAL(q);
    CU_ASSERT_EQUAL(q->nelts, 8);
    CU_ASSERT_EQUAL(q->top, 0);
    CU_ASSERT_EQUAL(q->bottom, 0);    
    CU_ASSERT_EQUAL(q->elts[0], 0);
    free_simple_queue(q);
    CU_ASSERT_EQUAL(q->elts[0], 0);
}


static void test_enqueue_dequeue(void)
{
    simple_queue *q = new_simple_queue(8);
    simple_queue *q1 = q;
    unsigned x;

    enqueue_simple_queue(&q, 1);
    CU_ASSERT_PTR_EQUAL(q, q1);
    CU_ASSERT_EQUAL(q->top, 1);
    CU_ASSERT_EQUAL(q->bottom, 0);
    x = dequeue_simple_queue(q);
    CU_ASSERT_EQUAL(q->top, 1);
    CU_ASSERT_EQUAL(q->bottom, 1);
    CU_ASSERT_EQUAL(x, 1);
    free_simple_queue(q);
}

static void test_enqueue_dequeue2(void)
{
    simple_queue *q = new_simple_queue(8);
    simple_queue *q1 = q;
    unsigned x;

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);    
    CU_ASSERT_PTR_EQUAL(q, q1);
    CU_ASSERT_EQUAL(q->top, 2);
    CU_ASSERT_EQUAL(q->bottom, 0);
    x = dequeue_simple_queue(q);
    CU_ASSERT_EQUAL(q->top, 2);
    CU_ASSERT_EQUAL(q->bottom, 1);
    CU_ASSERT_EQUAL(x, 1);
    free_simple_queue(q);
}

static void test_enqueue_dequeue_back(void)
{
    simple_queue *q = new_simple_queue(8);
    unsigned x;

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);
    x = dequeue_back_simple_queue(q);
    CU_ASSERT_EQUAL(q->top, 1);
    CU_ASSERT_EQUAL(q->bottom, 0);
    CU_ASSERT_EQUAL(x, 2);
    free_simple_queue(q);
}

static void test_enqueue_front(void)
{
    simple_queue *q = new_simple_queue(8);

    enqueue_front_simple_queue(&q, 1);
    CU_ASSERT_EQUAL(q->top, 1);
    CU_ASSERT_EQUAL(q->bottom, 0);
    enqueue_front_simple_queue(&q, 2);
    CU_ASSERT_EQUAL(q->top, 8);    
    CU_ASSERT_EQUAL(q->bottom, 6);    
    free_simple_queue(q);
}

static void test_enqueue_dequeue_enqueue_front(void)
{
    simple_queue *q = new_simple_queue(8);
    unsigned x;

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);
    dequeue_simple_queue(q);
    enqueue_front_simple_queue(&q, 3);
    CU_ASSERT_EQUAL(q->bottom, 0);
    CU_ASSERT_EQUAL(q->top, 2);
    x = dequeue_simple_queue(q);
    CU_ASSERT_EQUAL(x, 3);
    free_simple_queue(q);
}


static void test_enqueue_grow(void)
{
    simple_queue *q = new_simple_queue(1);
    simple_queue *q1 = q;
    unsigned x;

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);
    CU_ASSERT_EQUAL(q->top, 2);
    CU_ASSERT_PTR_NOT_EQUAL(q, q1);
    CU_ASSERT_EQUAL(q->nelts, 6);
    CU_ASSERT_EQUAL(q->elts[0], 1);
    q1 = q;
    enqueue_simple_queue(&q, 3);
    CU_ASSERT_PTR_EQUAL(q1, q);
    CU_ASSERT_EQUAL(q->nelts, 6);
    x = dequeue_simple_queue(q);
    CU_ASSERT_EQUAL(q->nelts, 6);
    CU_ASSERT_EQUAL(x, 1);
    x = dequeue_simple_queue(q);
    CU_ASSERT_EQUAL(x, 2);
    x = dequeue_simple_queue(q);
    CU_ASSERT_EQUAL(x, 3);

    free_simple_queue(q);
}

static void test_enqueue_front_grow(void)
{
    simple_queue *q = new_simple_queue(1);
    simple_queue *q1 = q;

    enqueue_front_simple_queue(&q, 1);
    enqueue_front_simple_queue(&q, 2);
    CU_ASSERT_PTR_NOT_EQUAL(q, q1);
    CU_ASSERT_EQUAL(q->top, 6);
    CU_ASSERT_EQUAL(q->bottom, 4);    
    CU_ASSERT_EQUAL(q->nelts, 6);
    CU_ASSERT_EQUAL(q->elts[4], 2);
    CU_ASSERT_EQUAL(q->elts[5], 1);

    free_simple_queue(q);
}


static void test_clear_queue(void)
{
    simple_queue *q = new_simple_queue(3);

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);
    enqueue_simple_queue(&q, 3);
    clear_simple_queue(q);
    CU_ASSERT_EQUAL(q->top, 0);
    CU_ASSERT_EQUAL(q->bottom, 0);    
    CU_ASSERT_EQUAL(q->nelts, 3);
    CU_ASSERT_EQUAL(q->elts[0], 0);
    CU_ASSERT_EQUAL(q->elts[1], 0);
    CU_ASSERT_EQUAL(q->elts[2], 0);

    free_simple_queue(q);
}

typedef struct queued_refcnt {
    unsigned refcnt;
    unsigned value;
} queued_refcnt;

DECLARE_REFCNT_ALLOCATOR(queued_refcnt, (unsigned val));
DECLARE_QUEUE_TYPE(refcnt_queue, queued_refcnt *);
DECLARE_QUEUE_REFCNT_OPS(refcnt_queue, queued_refcnt);

DEFINE_REFCNT_ALLOCATOR(queued_refcnt, (unsigned val), obj,
                        { obj->value = val;},
                        { NEW(obj)->value = OLD(obj)->value + 1; },
                        { obj->value = 0; });
DEFINE_QUEUE_REFCNT_OPS(refcnt_queue, queued_refcnt, linear, 8, 4);

static void test_enq_deq_refcnt(void)
{
    queued_refcnt *obj = new_queued_refcnt(0x12345);
    refcnt_queue *q = new_refcnt_queue(2);
    queued_refcnt *obj1, *obj2;

    enqueue_refcnt_queue(&q, obj);
    enqueue_front_refcnt_queue(&q, obj);    
    CU_ASSERT_EQUAL(obj->refcnt, 3);
    obj1 = dequeue_refcnt_queue(q);
    obj2 = dequeue_back_refcnt_queue(q);    
    CU_ASSERT_PTR_EQUAL(obj, obj1);
    CU_ASSERT_PTR_EQUAL(obj1, obj2);
    CU_ASSERT_EQUAL(obj1->refcnt, 3);
    
    free_queued_refcnt(obj);
    free_queued_refcnt(obj1);
    free_queued_refcnt(obj2);    
    free_refcnt_queue(q);
}
                        

static void test_queue_copy_refcnt(void)
{
    refcnt_queue *q = new_refcnt_queue(2);
    refcnt_queue *q1;
    queued_refcnt *obj = new_queued_refcnt(0x12345);

    enqueue_refcnt_queue(&q, obj);
    enqueue_refcnt_queue(&q, obj);
    free_queued_refcnt(obj);
    q1 = copy_refcnt_queue(q);
    CU_ASSERT_PTR_NOT_EQUAL(q1, q);
    CU_ASSERT_EQUAL(q1->top, 2);
    CU_ASSERT_EQUAL(obj->refcnt, 4);
    CU_ASSERT_EQUAL(obj->value, 0x12345);
    free_refcnt_queue(q1);
    free_refcnt_queue(q);
}

static void test_clear_refcnt_queue(void)
{
    refcnt_queue *q = new_refcnt_queue(3);
    queued_refcnt *obj = new_queued_refcnt(0x12345);

    enqueue_refcnt_queue(&q, obj);
    enqueue_refcnt_queue(&q, obj);
    enqueue_refcnt_queue(&q, obj);
    clear_refcnt_queue(q);
    CU_ASSERT_EQUAL(obj->refcnt, 1);

    free_refcnt_queue(q);
    CU_ASSERT_EQUAL(obj->refcnt, 1);
    free_queued_refcnt(obj);
}


test_suite_descr queue_tests = {
    "queue", NULL, NULL,
    (const test_descr[]){
        TEST_DESCR(alloc_queue),
        TEST_DESCR(enqueue_dequeue),
        TEST_DESCR(enqueue_dequeue2),
        TEST_DESCR(enqueue_dequeue_back),
        TEST_DESCR(enqueue_front),
        TEST_DESCR(enqueue_dequeue_enqueue_front),        
        TEST_DESCR(enqueue_grow),
        TEST_DESCR(enqueue_front_grow),        
        TEST_DESCR(clear_queue),
        TEST_DESCR(enq_deq_refcnt),
        TEST_DESCR(queue_copy_refcnt),
        TEST_DESCR(clear_refcnt_queue),
        {NULL, NULL}
    }
};
