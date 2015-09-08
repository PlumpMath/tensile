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
#define IMPLEMENT_TAGLIST
#include "taglist.h"

DECLARE_TAGLIST_TYPE(simple_taglist, unsigned);
DECLARE_TAGLIST_OPS(simple_taglist, unsigned);

DEFINE_TAGLIST_OPS(simple_taglist, unsigned, 16, list, i,
                   {list->elts[i].value = 0x12345; },
                   {NEW(list)->elts[i].value = OLD(list)->elts[i].value + 1;},
                   {list->elts[i].value = 0xdeadbeef; }, 4);

static void test_alloc_taglist(void)
{
    simple_taglist *list = new_simple_taglist(8);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    CU_ASSERT_EQUAL(list->nelts, 8);
    CU_ASSERT_EQUAL(list->elts[0].tag, NULL_TAG);
    CU_ASSERT_EQUAL(list->elts[0].value, 0x12345);
    free_simple_taglist(list);
    CU_ASSERT_EQUAL(list->elts[1].tag, NULL_TAG);
    CU_ASSERT_EQUAL(list->elts[1].value, 0xdeadbeef);
}

static void test_add_lookup(void)
{
    simple_taglist *list = new_simple_taglist(1);
    simple_taglist *saved = list;
    unsigned *found;

    *add_simple_taglist(&list, 1) = 123;
    CU_ASSERT_PTR_EQUAL(list, saved);
    found = lookup_simple_taglist(&list, 1, false);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    CU_ASSERT_EQUAL(*found, 123);
    free_simple_taglist(list);
}

static void test_add_grow(void)
{
    simple_taglist *list = new_simple_taglist(1);
    simple_taglist *saved = list;

    *add_simple_taglist(&list, 1) = 123;
    *add_simple_taglist(&list, 2) = 456;
    CU_ASSERT_PTR_NOT_EQUAL(list, saved);
    saved = list;
    *add_simple_taglist(&list, 3) = 789;
    CU_ASSERT_PTR_EQUAL(list, saved);

    free_simple_taglist(list);
}

static void test_lookup_add(void)
{
    simple_taglist *list = new_simple_taglist(1);
    unsigned *found;

    found = lookup_simple_taglist(&list, 1, true);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    CU_ASSERT_EQUAL(*found, 0x12345);
    free_simple_taglist(list);
}


static void test_lookup(void)
{
    simple_taglist *list = new_simple_taglist(4);
    unsigned *found;

    *add_simple_taglist(&list, 1) = 123;
    *add_simple_taglist(&list, 2) = 456;
    *add_simple_taglist(&list, 3) = 789;        
    found = lookup_simple_taglist(&list, 1, false);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    CU_ASSERT_EQUAL(*found, 123);
    found = lookup_simple_taglist(&list, 2, false);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    CU_ASSERT_EQUAL(*found, 456);
    found = lookup_simple_taglist(&list, 3, false);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    CU_ASSERT_EQUAL(*found, 789);
    found = lookup_simple_taglist(&list, 4, false);
    CU_ASSERT_PTR_NULL(found);
    free_simple_taglist(list);
}

static void test_lookupmulti(void)
{
    simple_taglist *list = new_simple_taglist(4);
    unsigned *found;

    *add_simple_taglist(&list, 1) = 123;
    *add_simple_taglist(&list, 2) = 456;
    *add_simple_taglist(&list, 1) = 789;        
    found = lookup_simple_taglist(&list, 1, false);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    CU_ASSERT_EQUAL(*found, 123);
    free_simple_taglist(list);
}

static void test_add_delete(void)
{
    simple_taglist *list = new_simple_taglist(1);
    unsigned *found;

    found = lookup_simple_taglist(&list, 1, true);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    delete_simple_taglist(list, 1, false);
    found = lookup_simple_taglist(&list, 1, false);
    CU_ASSERT_PTR_NULL(found);
    free_simple_taglist(list);
}

static void test_add_delete_add(void)
{
    simple_taglist *list = new_simple_taglist(1);
    simple_taglist *saved = list;
    unsigned *found;
    unsigned *added;

    found = lookup_simple_taglist(&list, 1, true);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    *found = 123;
    delete_simple_taglist(list, 1, false);
    added = add_simple_taglist(&list, 2);
    CU_ASSERT_PTR_EQUAL(list, saved);
    CU_ASSERT_PTR_EQUAL(found, added);
    CU_ASSERT_EQUAL(*added, 0x12345);
    free_simple_taglist(list);
}

static void test_add_add_delete_unhide(void)
{
    simple_taglist *list = new_simple_taglist(2);
    unsigned *found;

    found = lookup_simple_taglist(&list, 1, true);
    *found = 123;
    found = add_simple_taglist(&list, 1);
    *found = 234;    
    found = lookup_simple_taglist(&list, 1, false);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    CU_ASSERT_EQUAL(*found, 123);
    delete_simple_taglist(list, 1, false);
    found = lookup_simple_taglist(&list, 1, false);
    CU_ASSERT_PTR_NOT_NULL_FATAL(found);
    CU_ASSERT_EQUAL(*found, 234);    
    free_simple_taglist(list);
}

static void test_add_add_delete_all(void)
{
    simple_taglist *list = new_simple_taglist(3);
    unsigned *found;

    found = lookup_simple_taglist(&list, 1, true);
    *found = 123;
    found = add_simple_taglist(&list, 2);
    *found = 456;        
    found = add_simple_taglist(&list, 1);
    *found = 234;    
    delete_simple_taglist(list, 1, true);
    found = lookup_simple_taglist(&list, 1, false);
    CU_ASSERT_PTR_NULL(found);
    found = lookup_simple_taglist(&list, 2, false);    
    CU_ASSERT_EQUAL(*found, 456);    
    free_simple_taglist(list);
}

static void test_add_add_foreach(void)
{
    simple_taglist *list = new_simple_taglist(3);
    unsigned *found;
    unsigned count = 0;
    unsigned sum = 0;

    found = lookup_simple_taglist(&list, 1, true);
    *found = 123;
    found = add_simple_taglist(&list, 2);
    *found = 456;        
    found = add_simple_taglist(&list, 1);
    *found = 234;

    FOREACH_TAGGED(list, 1, i, 
                   {
                       count++;
                       sum += list->elts[i].value;
                   });
    CU_ASSERT_EQUAL(count, 2);
    CU_ASSERT_EQUAL(sum, 357);

    free_simple_taglist(list);
}

typedef struct tagged_refcnt {
    unsigned refcnt;
    unsigned value;
} tagged_refcnt;

DECLARE_REFCNT_ALLOCATOR(tagged_refcnt, (unsigned val));
DECLARE_TAGLIST_TYPE(refcnt_taglist, tagged_refcnt *);
DECLARE_TAGLIST_REFCNT_OPS(refcnt_taglist, tagged_refcnt);

DEFINE_REFCNT_ALLOCATOR(tagged_refcnt, (unsigned val),
                        obj, { obj->value = val; }, {},
                        { obj->value = 0xdeadbeef; });
DEFINE_TAGLIST_REFCNT_OPS(refcnt_taglist, tagged_refcnt, 16, 1);

static void test_get_none(void)
{
    refcnt_taglist *list = new_refcnt_taglist(4);

    CU_ASSERT_PTR_NULL(get_refcnt_taglist(list, 1));
    free_refcnt_taglist(list);
}

static void test_put_get(void)
{
    tagged_refcnt *obj = new_tagged_refcnt(0x12345);
    tagged_refcnt *got;
    refcnt_taglist *list = new_refcnt_taglist(4);

    got = put_refcnt_taglist(&list, 1, obj);
    CU_ASSERT_PTR_NULL(got);
    CU_ASSERT_EQUAL(obj->refcnt, 2);
    got = get_refcnt_taglist(list, 1);
    CU_ASSERT_PTR_EQUAL(got, obj);
    CU_ASSERT_EQUAL(obj->refcnt, 3);
    free_tagged_refcnt(got);    
    
    free_refcnt_taglist(list);
    CU_ASSERT_EQUAL(obj->refcnt, 1);    
    free_tagged_refcnt(obj);
}

static void test_replace(void)
{
    tagged_refcnt *obj = new_tagged_refcnt(0x12345);
    tagged_refcnt *obj1 = new_tagged_refcnt(0x12346);
    tagged_refcnt *got;
    refcnt_taglist *list = new_refcnt_taglist(4);

    replace_refcnt_taglist(&list, 1, obj);
    replace_refcnt_taglist(&list, 1, obj1);
    CU_ASSERT_EQUAL(obj->refcnt, 1);
    got = get_refcnt_taglist(list, 1);
    CU_ASSERT_PTR_EQUAL(got, obj1);
    free_tagged_refcnt(got);    
    free_tagged_refcnt(obj1);    
    free_refcnt_taglist(list);
    free_tagged_refcnt(obj);
}


static void test_copy_refcnt(void)
{
    tagged_refcnt *obj = new_tagged_refcnt(0x12345);
    refcnt_taglist *list = new_refcnt_taglist(4);
    refcnt_taglist *copy;

    replace_refcnt_taglist(&list, 1, obj);
    copy = copy_refcnt_taglist(list);
    CU_ASSERT_PTR_NOT_NULL_FATAL(copy);
    CU_ASSERT_EQUAL(copy->nelts, list->nelts);
    
}

test_suite_descr taglist_tests = {
    "taglist", NULL, NULL,
    (const test_descr[]){
        TEST_DESCR(alloc_taglist),
        TEST_DESCR(add_lookup),
        TEST_DESCR(add_grow),
        TEST_DESCR(lookup_add),
        TEST_DESCR(lookup),
        TEST_DESCR(lookupmulti),
        TEST_DESCR(add_delete),
        TEST_DESCR(add_delete_add),
        TEST_DESCR(add_add_delete_unhide),
        TEST_DESCR(add_add_delete_all),
        TEST_DESCR(add_add_foreach),
        TEST_DESCR(get_none),
        TEST_DESCR(put_get),
        TEST_DESCR(replace),
        {NULL, NULL}
    }
};
