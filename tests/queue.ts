#include <stdlib.h>
#define IMPLEMENT_QUEUE 1
#include "queue.h"

DECLARE_QUEUE_TYPE(simple_queue, unsigned);
DECLARE_QUEUE_OPS(simple_queue, unsigned, trivial_dequeue);

DEFINE_QUEUE_OPS(simple_queue, unsigned, linear, 16, q, i,
                 {q->elts[i] = 0; },
                 {NEW(q)->elts[i] = OLD(q)->elts[i] + 1;},
                 {q->elts[i] = 0xdeadbeef; },
                 trivial_enqueue, 4);

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


#suite Queue
#test alloc_queue
    simple_queue *q = new_simple_queue(8);

    ck_assert_ptr_ne(q, NULL);
    ck_assert_uint_eq(q->nelts, 8);
    ck_assert_uint_eq(q->top, 0);
    ck_assert_uint_eq(q->bottom, 0);    
    ck_assert_uint_eq(q->elts[0], 0);
    free_simple_queue(q);
    ck_assert_uint_eq(q->elts[0], 0);


#test enqueue_dequeue
    simple_queue *q = new_simple_queue(8);
    simple_queue *q1 = q;
    unsigned x;

    enqueue_simple_queue(&q, 1);
    ck_assert_ptr_eq(q, q1);
    ck_assert_uint_eq(q->top, 1);
    ck_assert_uint_eq(q->bottom, 0);
    x = dequeue_simple_queue(q);
    ck_assert_uint_eq(q->top, 1);
    ck_assert_uint_eq(q->bottom, 1);
    ck_assert_uint_eq(x, 1);
    free_simple_queue(q);

#test enqueue_dequeue2
    simple_queue *q = new_simple_queue(8);
    simple_queue *q1 = q;
    unsigned x;

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);    
    ck_assert_ptr_eq(q, q1);
    ck_assert_uint_eq(q->top, 2);
    ck_assert_uint_eq(q->bottom, 0);
    x = dequeue_simple_queue(q);
    ck_assert_uint_eq(q->top, 2);
    ck_assert_uint_eq(q->bottom, 1);
    ck_assert_uint_eq(x, 1);
    free_simple_queue(q);

#test enqueue_dequeue_back
    simple_queue *q = new_simple_queue(8);
    unsigned x;

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);
    x = dequeue_back_simple_queue(q);
    ck_assert_uint_eq(q->top, 1);
    ck_assert_uint_eq(q->bottom, 0);
    ck_assert_uint_eq(x, 2);
    free_simple_queue(q);

#test enqueue_front
    simple_queue *q = new_simple_queue(8);

    enqueue_front_simple_queue(&q, 1);
    ck_assert_uint_eq(q->top, 1);
    ck_assert_uint_eq(q->bottom, 0);
    enqueue_front_simple_queue(&q, 2);
    ck_assert_uint_eq(q->top, 8);    
    ck_assert_uint_eq(q->bottom, 6);    
    free_simple_queue(q);

#test enqueue_dequeue_enqueue_front
    simple_queue *q = new_simple_queue(8);
    unsigned x;

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);
    dequeue_simple_queue(q);
    enqueue_front_simple_queue(&q, 3);
    ck_assert_uint_eq(q->bottom, 0);
    ck_assert_uint_eq(q->top, 2);
    x = dequeue_simple_queue(q);
    ck_assert_uint_eq(x, 3);
    free_simple_queue(q);


#test enqueue_grow
    simple_queue *q = new_simple_queue(1);
    simple_queue *q1 = q;
    unsigned x;

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);
    ck_assert_uint_eq(q->top, 2);
    ck_assert_ptr_ne(q, q1);
    ck_assert_uint_eq(q->nelts, 6);
    ck_assert_uint_eq(q->elts[0], 1);
    q1 = q;
    enqueue_simple_queue(&q, 3);
    ck_assert_ptr_eq(q1, q);
    ck_assert_uint_eq(q->nelts, 6);
    x = dequeue_simple_queue(q);
    ck_assert_uint_eq(q->nelts, 6);
    ck_assert_uint_eq(x, 1);
    x = dequeue_simple_queue(q);
    ck_assert_uint_eq(x, 2);
    x = dequeue_simple_queue(q);
    ck_assert_uint_eq(x, 3);

    free_simple_queue(q);

#test enqueue_front_grow
    simple_queue *q = new_simple_queue(1);
    simple_queue *q1 = q;

    enqueue_front_simple_queue(&q, 1);
    enqueue_front_simple_queue(&q, 2);
    ck_assert_ptr_ne(q, q1);
    ck_assert_uint_eq(q->top, 6);
    ck_assert_uint_eq(q->bottom, 4);    
    ck_assert_uint_eq(q->nelts, 6);
    ck_assert_uint_eq(q->elts[4], 2);
    ck_assert_uint_eq(q->elts[5], 1);

    free_simple_queue(q);


#test clear_queue
    simple_queue *q = new_simple_queue(3);

    enqueue_simple_queue(&q, 1);
    enqueue_simple_queue(&q, 2);
    enqueue_simple_queue(&q, 3);
    clear_simple_queue(q);
    ck_assert_uint_eq(q->top, 0);
    ck_assert_uint_eq(q->bottom, 0);    
    ck_assert_uint_eq(q->nelts, 3);
    ck_assert_uint_eq(q->elts[0], 0);
    ck_assert_uint_eq(q->elts[1], 0);
    ck_assert_uint_eq(q->elts[2], 0);

    free_simple_queue(q);

#test enq_deq_refcnt
    queued_refcnt *obj = new_queued_refcnt(0x12345);
    refcnt_queue *q = new_refcnt_queue(2);
    queued_refcnt *obj1, *obj2;

    enqueue_refcnt_queue(&q, obj);
    enqueue_front_refcnt_queue(&q, obj);    
    ck_assert_uint_eq(obj->refcnt, 3);
    obj1 = dequeue_refcnt_queue(q);
    obj2 = dequeue_back_refcnt_queue(q);    
    ck_assert_ptr_eq(obj, obj1);
    ck_assert_ptr_eq(obj1, obj2);
    ck_assert_uint_eq(obj1->refcnt, 3);
    
    free_queued_refcnt(obj);
    free_queued_refcnt(obj1);
    free_queued_refcnt(obj2);    
    free_refcnt_queue(q);
                        

#test queue_copy_refcnt
    refcnt_queue *q = new_refcnt_queue(2);
    refcnt_queue *q1;
    queued_refcnt *obj = new_queued_refcnt(0x12345);

    enqueue_refcnt_queue(&q, obj);
    enqueue_refcnt_queue(&q, obj);
    free_queued_refcnt(obj);
    q1 = copy_refcnt_queue(q);
    ck_assert_ptr_ne(q1, q);
    ck_assert_uint_eq(q1->top, 2);
    ck_assert_uint_eq(obj->refcnt, 4);
    ck_assert_uint_eq(obj->value, 0x12345);
    free_refcnt_queue(q1);
    free_refcnt_queue(q);

#test ts_clear_refcnt_queue
    refcnt_queue *q = new_refcnt_queue(3);
    queued_refcnt *obj = new_queued_refcnt(0x12345);

    enqueue_refcnt_queue(&q, obj);
    enqueue_refcnt_queue(&q, obj);
    enqueue_refcnt_queue(&q, obj);
    clear_refcnt_queue(q);
    ck_assert_uint_eq(obj->refcnt, 1);

    free_refcnt_queue(q);
    ck_assert_uint_eq(obj->refcnt, 1);
    free_queued_refcnt(obj);

