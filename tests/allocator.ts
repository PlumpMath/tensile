
#include <stdlib.h>
#define IMPLEMENT_ALLOCATOR 1
#include "allocator.h"

typedef struct simple_type {
    void *ptr;
    unsigned tag;
} simple_type;

DECLARE_TYPE_ALLOCATOR(simple_type, (unsigned tag));

DEFINE_TYPE_ALLOCATOR(simple_type, (unsigned tag), obj,
                      { obj->tag = tag; },
                      { NEW(obj)->tag = OLD(obj)->tag + 1; },
                      { obj->tag = 0xdeadbeef; });

typedef struct simple_type2 {
    void *ptr;
} simple_type2;


DECLARE_TYPE_ALLOCATOR(simple_type2, ());
DECLARE_TYPE_PREALLOC(simple_type2);

DEFINE_TYPE_ALLOCATOR(simple_type2, (), obj,
                      {},
                      {},
                      {});
DEFINE_TYPE_PREALLOC(simple_type2);

typedef short small_type;

DECLARE_TYPE_ALLOCATOR(small_type, ());
DEFINE_TYPE_ALLOCATOR(small_type, (), obj,
                      {*obj = 0x1234;},
                      {}, {});


typedef struct refcnt_type {
    unsigned refcnt;
    void *ptr;
    unsigned tag;
} refcnt_type;

DECLARE_REFCNT_ALLOCATOR(refcnt_type, (unsigned tag));
DEFINE_REFCNT_ALLOCATOR(refcnt_type, (unsigned tag), obj,
                        {obj->tag = tag;},
                        {NEW(obj)->tag = OLD(obj)->tag + 1;},
                        {obj->tag = 0xdeadbeef;});


DECLARE_ARRAY_TYPE(simple_array, void *ptr; unsigned tag;, unsigned);
DECLARE_ARRAY_ALLOCATOR(simple_array);


DEFINE_ARRAY_ALLOCATOR(simple_array, linear, 4, arr, i,
                       {arr->tag = 0x12345;},
                       {arr->elts[i] = i; },
                       {NEW(arr)->tag = OLD(arr)->tag + 1; },
                       {NEW(arr)->elts[i] = OLD(arr)->elts[i] + 1; },
                       {NEW(arr)->tag = OLD(arr)->tag << 4; },
                       {NEW(arr)->elts[i] = OLD(arr)->elts[i] << 4; },
                       {arr->tag |= 0x80000000u; },
                       {arr->tag = 0xdeadbeef; },
                       {arr->elts[i] = 0xdeadbeef; });


DECLARE_ARRAY_TYPE(simple_log_array, void *ptr; unsigned tag;, unsigned);
DECLARE_ARRAY_ALLOCATOR(simple_log_array);


DEFINE_ARRAY_ALLOCATOR(simple_log_array, log2, 4, arr, i,
                       {arr->tag = 0x12345;},
                       {arr->elts[i] = i; },
                       {NEW(arr)->tag = OLD(arr)->tag + 1; },
                       {NEW(arr)->elts[i] = OLD(arr)->elts[i] + 1; },
                       {NEW(arr)->tag = OLD(arr)->tag << 4; },
                       {NEW(arr)->elts[i] = OLD(arr)->elts[i] << 4; },
                       {arr->tag |= 0x80000000u; },
                       {arr->tag = 0xdeadbeef; },
                       {arr->elts[i] = 0xdeadbeef; });


DEFINE_LINEAR_SCALE(2);

DECLARE_ARRAY_TYPE(simple_linear2_array, void *ptr; unsigned tag;, unsigned);
DECLARE_ARRAY_ALLOCATOR(simple_linear2_array);


DEFINE_ARRAY_ALLOCATOR(simple_linear2_array, linear2, 4, arr, i,
                       {arr->tag = 0x12345;},
                       {arr->elts[i] = i; },
                       {NEW(arr)->tag = OLD(arr)->tag + 1; },
                       {NEW(arr)->elts[i] = OLD(arr)->elts[i] + 1; },
                       {NEW(arr)->tag = OLD(arr)->tag << 4; },
                       {NEW(arr)->elts[i] = OLD(arr)->elts[i] << 4; },
                       {arr->tag |= 0x80000000u; },
                       {arr->tag = 0xdeadbeef; },
                       {arr->elts[i] = 0xdeadbeef; });



DECLARE_ARRAY_TYPE(refcnt_array,
                   void *ptr; unsigned refcnt; unsigned tag;,
                   unsigned);

DECLARE_REFCNT_ARRAY_ALLOCATOR(refcnt_array);
DEFINE_REFCNT_ARRAY_ALLOCATOR(refcnt_array,
                              linear, 4, arr, i,
                              {arr->tag = 0x12345;},
                              {},
                              {},
                              {},
                              {},
                              {},
                              {},
                              {arr->tag = 0xdeadbeef; },
                              {});

static void test_resize_smaller_n(unsigned n)
{
    simple_array *arr = new_simple_array(n);
    simple_array *arr1 = resize_simple_array(arr, n - 1);
    unsigned j;

    ck_assert_ptr_eq(arr, arr1);
    ck_assert_uint_eq(arr->nelts, n - 1);
    ck_assert_uint_eq(arr->tag, 0x80000000 | 0x12345);
    for (j = 0; j < n - 1; j++)
        ck_assert_uint_eq(arr->elts[j], j);
    ck_assert_uint_eq(arr->elts[n - 1], 0xdeadbeef);

    free_simple_array(arr);
}

static void test_resize_larger_n(unsigned n)
{
    simple_array *arr = new_simple_array(n);
    simple_array *arr1 = resize_simple_array(arr, n + 1);
    unsigned j;

    ck_assert_ptr_ne(arr, arr1);
    ck_assert_uint_eq(arr1->nelts, n + 1);
    ck_assert_uint_eq(arr1->tag, 0x80000000 | 0x123450);
    for (j = 0; j < n; j++)
        ck_assert_uint_eq(arr1->elts[j], j << 4);
    ck_assert_uint_eq(arr1->elts[n], n);

    free_simple_array(arr1);
}

#suite Allocator

#test alloc_init
    simple_type *st = new_simple_type(0x12345);

    ck_assert_ptr_ne(st, NULL);
    ck_assert_uint_eq(st->tag, 0x12345);
    free_simple_type(st);

#test destroy
    simple_type *st = new_simple_type(0x12345);

    free_simple_type(st);
    ck_assert_uint_eq(st->tag, 0xdeadbeef);

#test destroy_alloc
    simple_type *st = new_simple_type(0x12345);
    simple_type *st1;

    free_simple_type(st);
    st1 = new_simple_type(0x54321);
    ck_assert_ptr_eq(st, st1);
    ck_assert_uint_eq(st1->tag, 0x54321);
    free_simple_type(st1);

#test destroy_alloc_alloc
    simple_type *st = new_simple_type(0x12345);
    simple_type *st1;
    simple_type *st2;

    free_simple_type(st);
    st1 = new_simple_type(0x54321);
    ck_assert_ptr_eq(st, st1);
    st2 = new_simple_type(0);
    ck_assert_ptr_ne(st2, st1);
    free_simple_type(st1);
    free_simple_type(st2);


#test copy
    simple_type *st = new_simple_type(0x12345);
    simple_type *st1;

    st->ptr = &st;
    st1 = copy_simple_type(st);
    ck_assert_ptr_ne(st, st1);
    ck_assert_ptr_eq(st1->ptr, st->ptr);
    ck_assert_uint_eq(st1->tag, 0x12346);
    free_simple_type(st);
    free_simple_type(st1);


#test dealloc_copy
    simple_type *st = new_simple_type(0x12345);
    simple_type *st1 = new_simple_type(0);
    simple_type *st2;

    free_simple_type(st);
    st2 = copy_simple_type(st1);
    ck_assert_ptr_eq(st2, st);
    free_simple_type(st1);
    free_simple_type(st2);

#test prealloc
    simple_type2 *st;
    simple_type2 *st1;
    simple_type2 *st2;

    preallocate_simple_type2s(2);
    st = new_simple_type2();
    st1 = new_simple_type2();
    st2 = new_simple_type2();
    ck_assert_ptr_eq(st1, st + 1);
    ck_assert_ptr_ne(st2, st);
    ck_assert_ptr_ne(st2, st1);
    free_simple_type2(st);
    free_simple_type2(st1);
    free_simple_type2(st2);


#test alloc_small
    small_type *sm;
    small_type *sm1;

    sm = new_small_type();
    ck_assert_int_eq(*sm, 0x1234);
    free_small_type(sm);
    sm1 = new_small_type();
    ck_assert_ptr_eq(sm, sm1);
    ck_assert_int_eq(*sm1, 0x1234);
    free_small_type(sm1);


#test refcnt_alloc_init
    refcnt_type *rt = new_refcnt_type(0x12345);

    ck_assert_ptr_ne(rt, NULL);
    ck_assert_uint_eq(rt->tag, 0x12345);
    ck_assert_uint_eq(rt->refcnt, 1);
    free_refcnt_type(rt);

#test refcnt_destroy
    refcnt_type *rt = new_refcnt_type(0x12345);

    free_refcnt_type(rt);
    ck_assert_uint_eq(rt->tag, 0xdeadbeef);

#test refcnt_use_destroy
    refcnt_type *rt = new_refcnt_type(0x12345);
    refcnt_type *use = use_refcnt_type(rt);

    ck_assert_ptr_eq(use, rt);
    ck_assert_uint_eq(use->refcnt, 2);
    free_refcnt_type(rt);
    ck_assert_uint_eq(use->refcnt, 1);
    ck_assert_uint_eq(use->tag, 0x12345);
    free_refcnt_type(use);
    ck_assert_uint_eq(use->tag, 0xdeadbeef);

#test refcnt_destroy_alloc
    refcnt_type *rt = new_refcnt_type(0x12345);
    refcnt_type *rt1;

    free_refcnt_type(rt);
    rt1 = new_refcnt_type(0x54321);
    ck_assert_ptr_eq(rt, rt1);
    ck_assert_uint_eq(rt1->refcnt, 1);
    ck_assert_uint_eq(rt1->tag, 0x54321);
    free_refcnt_type(rt1);
    ck_assert_uint_eq(rt1->tag, 0xdeadbeef);

#test refcnt_copy
    refcnt_type *rt = new_refcnt_type(0x12345);
    refcnt_type *rt1 = copy_refcnt_type(rt);

    ck_assert_ptr_ne(rt, rt1);
    ck_assert_uint_eq(rt->refcnt, 1);
    ck_assert_uint_eq(rt1->refcnt, 1);
    ck_assert_uint_eq(rt1->tag, 0x12346);
    free_refcnt_type(rt1);
    free_refcnt_type(rt);


#test alloc_sizes
    unsigned i;
    simple_array *prev = NULL;

    for (i = 0; i < 5; i++)
    {
        unsigned j;
        simple_array *arr = new_simple_array(i);

        ck_assert_ptr_ne(arr, NULL);
        ck_assert_ptr_ne(arr, prev);
        ck_assert_uint_eq(arr->tag, 0x12345);
        ck_assert_uint_eq(arr->nelts, i);
        for (j = 0; j < i; j++)
            ck_assert_uint_eq(arr->elts[j], j);

        free_simple_array(arr);
        if (i != 4)
        {
            ck_assert_uint_eq(arr->tag, 0xdeadbeef);
            for (j = 0; j < i; j++)
                ck_assert_uint_eq(arr->elts[j], 0xdeadbeef);
        }
        prev = arr;
    }


#test alloc_free_sizes
    unsigned i;

    for (i = 0; i < 4; i++)
    {
        simple_array *arr = new_simple_array(i);
        simple_array *arr1;

        free_simple_array(arr);
        arr1 = new_simple_array(i);
        ck_assert_ptr_eq(arr, arr1);
        free_simple_array(arr1);
    }

#test copy_sizes
    unsigned i;

    for (i = 0; i <= 4; i++)
    {
        simple_array *arr = new_simple_array(i);
        simple_array *arr1 = copy_simple_array(arr);
        unsigned j;

        ck_assert_ptr_ne(arr, arr1);
        ck_assert_uint_eq(arr1->tag, arr->tag + 1);
        for (j = 0; j < i; j++)
            ck_assert_uint_eq(arr1->elts[j], arr->elts[j] + 1);
        free_simple_array(arr1);
        ck_assert_uint_eq(arr->tag, 0x12345);
        free_simple_array(arr);
    }

#test resize_null
    simple_array *arr = resize_simple_array(NULL, 3);

    ck_assert_ptr_ne(arr, NULL);
    ck_assert_uint_eq(arr->nelts, 3);
    ck_assert_uint_eq(arr->tag, 0x12345);
    ck_assert_uint_eq(arr->elts[2], 2);

    free_simple_array(arr);

#test resize_smaller
    test_resize_smaller_n(3);
    test_resize_smaller_n(4);
    test_resize_smaller_n(5);

#test resize_larger
    test_resize_larger_n(2);
    test_resize_larger_n(3);
    test_resize_larger_n(4);

#test resize_larger_free
    simple_array *arr = new_simple_array(2);
    simple_array *arr1 = resize_simple_array(arr, 3);
    simple_array *arr2 = new_simple_array(2);

    ck_assert_ptr_eq(arr2, arr);
    free_simple_array(arr1);
    free_simple_array(arr2);

#test log2order
    ck_assert_uint_eq(log2_order(0), 0);
    ck_assert_uint_eq(log2_order(1), 0);
    ck_assert_uint_eq(log2_order(2), 1);
    ck_assert_uint_eq(log2_order(3), 2);
    ck_assert_uint_eq(log2_order(4), 2);
    ck_assert_uint_eq(log2_order(5), 3);
    ck_assert_uint_eq(log2_order(65536), 16);

#test alloc_log2sizes
    unsigned i;
    simple_log_array *prev = NULL;

    for (i = 0; i < 5; i++)
    {
        simple_log_array *arr = new_simple_log_array(1u << i);

        ck_assert_ptr_ne(arr, NULL);
        ck_assert_ptr_ne(arr, prev);
        ck_assert_uint_eq(arr->tag, 0x12345);
        ck_assert_uint_eq(arr->nelts, 1u << i);

        free_simple_log_array(arr);
        if (i != 4)
        {
            ck_assert_uint_eq(arr->tag, 0xdeadbeef);
        }
        prev = arr;
    }

#test alloc_free_log2sizes
    unsigned i;

    for (i = 2; i < 4; i++)
    {
        simple_log_array *arr = new_simple_log_array(1u << i);
        simple_log_array *arr1;

        free_simple_log_array(arr);
        arr1 = new_simple_log_array((1u << i) - 1);
        ck_assert_ptr_eq(arr, arr1);
        free_simple_log_array(arr1);
    }

#test resize_larger_log2
    simple_log_array *arr = new_simple_log_array(9);
    simple_log_array *arr1 = resize_simple_log_array(arr, 10);

    ck_assert_ptr_eq(arr, arr1);
    ck_assert_uint_eq(arr1->nelts, 10);
    ck_assert_uint_eq(arr1->elts[9], 9);

    free_simple_log_array(arr1);


#test linear2order
    ck_assert_uint_eq(linear2_order(0), 0);
    ck_assert_uint_eq(linear2_order(1), 0);
    ck_assert_uint_eq(linear2_order(2), 1);
    ck_assert_uint_eq(linear2_order(3), 1);
    ck_assert_uint_eq(linear2_order(4), 2);
    ck_assert_uint_eq(linear2_order(5), 2);
    ck_assert_uint_eq(linear2_order(65536), 32768);

#test alloc_linear2sizes
    unsigned i;
    simple_linear2_array *prev = NULL;

    for (i = 0; i < 5; i++)
    {
        simple_linear2_array *arr = new_simple_linear2_array(i * 2);

        ck_assert_ptr_ne(arr, NULL);
        ck_assert_ptr_ne(arr, prev);
        ck_assert_uint_eq(arr->tag, 0x12345);
        ck_assert_uint_eq(arr->nelts, i * 2);

        free_simple_linear2_array(arr);
        if (i != 4)
        {
            ck_assert_uint_eq(arr->tag, 0xdeadbeef);
        }
        prev = arr;
    }

#test alloc_free_linear2sizes
    unsigned i;

    for (i = 0; i < 4; i++)
    {
        simple_linear2_array *arr = new_simple_linear2_array(i * 2);
        simple_linear2_array *arr1;

        free_simple_linear2_array(arr);
        arr1 = new_simple_linear2_array(i * 2 + 1);
        ck_assert_ptr_eq(arr, arr1);
        free_simple_linear2_array(arr1);
    }

#test resize_larger_linear2
    simple_linear2_array *arr = new_simple_linear2_array(8);
    simple_linear2_array *arr1 = resize_simple_linear2_array(arr, 9);

    ck_assert_ptr_eq(arr, arr1);
    ck_assert_uint_eq(arr1->nelts, 9);
    ck_assert_uint_eq(arr1->elts[8], 8);

    free_simple_linear2_array(arr1);

#test refcnt_array_alloc_init_destroy
    refcnt_array *rt = new_refcnt_array(2);

    ck_assert_ptr_ne(rt, NULL);
    ck_assert_uint_eq(rt->tag, 0x12345);
    ck_assert_uint_eq(rt->nelts, 2);
    ck_assert_uint_eq(rt->refcnt, 1);
    free_refcnt_array(rt);
    ck_assert_uint_eq(rt->tag, 0xdeadbeef);

#test refcnt_array_use_destroy
    refcnt_array *rt = new_refcnt_array(2);
    refcnt_array *use = use_refcnt_array(rt);

    ck_assert_ptr_eq(use, rt);
    ck_assert_uint_eq(use->refcnt, 2);
    free_refcnt_array(rt);
    ck_assert_uint_eq(use->refcnt, 1);
    ck_assert_uint_eq(use->tag, 0x12345);
    free_refcnt_array(use);
    ck_assert_uint_eq(use->tag, 0xdeadbeef);

#test refcnt_array_copy
    refcnt_array *rt = new_refcnt_array(2);
    refcnt_array *rt1 = copy_refcnt_array(rt);

    ck_assert_ptr_ne(rt, rt1);
    ck_assert_uint_eq(rt->refcnt, 1);
    ck_assert_uint_eq(rt1->refcnt, 1);
    ck_assert_uint_eq(rt1->tag, 0x12345);
    ck_assert_uint_eq(rt1->nelts, 2);
    free_refcnt_array(rt1);
    free_refcnt_array(rt);

#test ensure_size
    simple_array *arr = new_simple_array(2);
    simple_array *arr1 = NULL;

    arr1 = ensure_simple_array_size(arr, 1, 1);
    ck_assert_ptr_eq(arr, arr1);
    ck_assert_uint_eq(arr->nelts, 2);
    arr1 = ensure_simple_array_size(arr, 3, 2);
    ck_assert_ptr_ne(arr, arr1);
    ck_assert_uint_eq(arr1->nelts, 5);

    free_simple_array(arr1);
