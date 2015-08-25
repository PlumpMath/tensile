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

static void test_alloc_init(void) 
{
    simple_type *st = new_simple_type(0x12345);

    CU_ASSERT_PTR_NOT_NULL_FATAL(st);
    CU_ASSERT_EQUAL(st->tag, 0x12345);
    free_simple_type(st);
}

static void test_destroy(void) 
{
    simple_type *st = new_simple_type(0x12345);

    free_simple_type(st);
    CU_ASSERT_EQUAL(st->tag, 0xdeadbeef);
}

static void test_destroy_alloc(void) 
{
    simple_type *st = new_simple_type(0x12345);
    simple_type *st1;

    free_simple_type(st);
    st1 = new_simple_type(0x54321);
    CU_ASSERT_PTR_EQUAL(st, st1);
    CU_ASSERT_EQUAL(st1->tag, 0x54321);
    free_simple_type(st1);
}

static void test_destroy_alloc_alloc(void) 
{
    simple_type *st = new_simple_type(0x12345);
    simple_type *st1;
    simple_type *st2;

    free_simple_type(st);
    st1 = new_simple_type(0x54321);
    CU_ASSERT_PTR_EQUAL(st, st1);
    st2 = new_simple_type(0);
    CU_ASSERT_PTR_NOT_EQUAL(st2, st1);    
    free_simple_type(st1);
    free_simple_type(st2);
}


static void test_copy(void) 
{
    simple_type *st = new_simple_type(0x12345);
    simple_type *st1;

    st->ptr = &st;
    st1 = copy_simple_type(st);
    CU_ASSERT_PTR_NOT_EQUAL(st, st1);
    CU_ASSERT_PTR_EQUAL(st1->ptr, st->ptr);
    CU_ASSERT_EQUAL(st1->tag, 0x12346);
    free_simple_type(st);
    free_simple_type(st1);
}


static void test_dealloc_copy(void) 
{
    simple_type *st = new_simple_type(0x12345);
    simple_type *st1 = new_simple_type(0);
    simple_type *st2;

    free_simple_type(st);
    st2 = copy_simple_type(st1);
    CU_ASSERT_PTR_EQUAL(st2, st);
    free_simple_type(st1);
    free_simple_type(st2);
}


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


static void test_prealloc(void) 
{
    simple_type2 *st;
    simple_type2 *st1;
    simple_type2 *st2;    
    
    preallocate_simple_type2s(2);
    st = new_simple_type2();
    st1 = new_simple_type2();
    st2 = new_simple_type2();
    CU_ASSERT_PTR_EQUAL(st1, st + 1);
    CU_ASSERT_PTR_NOT_EQUAL(st2, st);
    CU_ASSERT_PTR_NOT_EQUAL(st2, st1);
    free_simple_type2(st);
    free_simple_type2(st1);
    free_simple_type2(st2);
}

typedef short small_type;

DECLARE_TYPE_ALLOCATOR(small_type, ());
DEFINE_TYPE_ALLOCATOR(small_type, (), obj, 
                      {*obj = 0x1234;},
                      {}, {});

static void test_alloc_small(void)
{
    small_type *sm;
    small_type *sm1;    
    
    sm = new_small_type();
    CU_ASSERT_EQUAL(*sm, 0x1234);
    free_small_type(sm);
    sm1 = new_small_type();
    CU_ASSERT_PTR_EQUAL(sm, sm1);
    CU_ASSERT_EQUAL(*sm1, 0x1234);
    free_small_type(sm1);
}                     
                      

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

static void test_refcnt_alloc_init(void) 
{
    refcnt_type *rt = new_refcnt_type(0x12345);

    CU_ASSERT_PTR_NOT_NULL_FATAL(rt);
    CU_ASSERT_EQUAL(rt->tag, 0x12345);
    CU_ASSERT_EQUAL(rt->refcnt, 1);
    free_refcnt_type(rt);
}

static void test_refcnt_destroy(void) 
{
    refcnt_type *rt = new_refcnt_type(0x12345);

    free_refcnt_type(rt);
    CU_ASSERT_EQUAL(rt->tag, 0xdeadbeef);
}

static void test_refcnt_use_destroy(void) 
{
    refcnt_type *rt = new_refcnt_type(0x12345);
    refcnt_type *use = use_refcnt_type(rt);

    CU_ASSERT_PTR_EQUAL(use, rt);
    CU_ASSERT_EQUAL(use->refcnt, 2);
    free_refcnt_type(rt);
    CU_ASSERT_EQUAL(use->refcnt, 1);
    CU_ASSERT_EQUAL(use->tag, 0x12345);
    free_refcnt_type(use);
    CU_ASSERT_EQUAL(use->tag, 0xdeadbeef);    
}

static void test_refcnt_destroy_alloc(void) 
{
    refcnt_type *rt = new_refcnt_type(0x12345);
    refcnt_type *rt1;

    free_refcnt_type(rt);
    rt1 = new_refcnt_type(0x54321);
    CU_ASSERT_PTR_EQUAL(rt, rt1);
    CU_ASSERT_EQUAL(rt1->refcnt, 1);
    CU_ASSERT_EQUAL(rt1->tag, 0x54321);
    free_refcnt_type(rt1);
    CU_ASSERT_EQUAL(rt1->tag, 0xdeadbeef);    
}

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

static void test_alloc_sizes(void)
{
    unsigned i;
    simple_array *prev = NULL;

    for (i = 0; i <= 5; i++) 
    {
        unsigned j;
        simple_array *arr = new_simple_array(i);

        CU_ASSERT_PTR_NOT_NULL_FATAL(arr);
        CU_ASSERT_PTR_NOT_EQUAL(arr, prev);
        CU_ASSERT_EQUAL(arr->tag, 0x12345);
        CU_ASSERT_EQUAL(arr->nelts, i);
        for (j = 0; j < i; j++)
            CU_ASSERT_EQUAL(arr->elts[j], j);

        free_simple_array(arr);
        if (i != 5) 
        {
            CU_ASSERT_EQUAL(arr->tag, 0xdeadbeef);
            for (j = 0; j < i; j++)
                CU_ASSERT_EQUAL(arr->elts[j], 0xdeadbeef);
        }
        prev = arr;
    }
}

test_suite_descr allocator_tests = {
    "allocator", NULL, NULL,
    (const test_descr[]){
        TEST_DESCR(alloc_init),
        TEST_DESCR(destroy),
        TEST_DESCR(destroy_alloc),
        TEST_DESCR(destroy_alloc_alloc),        
        TEST_DESCR(copy),
        TEST_DESCR(dealloc_copy),
        TEST_DESCR(prealloc),
        TEST_DESCR(alloc_small),
        TEST_DESCR(refcnt_alloc_init),
        TEST_DESCR(refcnt_destroy),
        TEST_DESCR(refcnt_use_destroy),
        TEST_DESCR(refcnt_destroy_alloc),
        TEST_DESCR(alloc_sizes),
        {NULL, NULL}
    }
};

