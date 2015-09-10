#include <stdlib.h>
#define IMPLEMENT_TAGLIST
#include "taglist.h"

DECLARE_TAGLIST_TYPE(simple_taglist, unsigned);
DECLARE_TAGLIST_OPS(simple_taglist, unsigned);

DEFINE_TAGLIST_OPS(simple_taglist, unsigned, 16, list, i,
                   {list->elts[i].value = 0x12345; },
                   {NEW(list)->elts[i].value = OLD(list)->elts[i].value + 1;},
                   {list->elts[i].value = 0xdeadbeef; }, 4);

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


#suite Taglist
#test alloc_taglist
    simple_taglist *list = new_simple_taglist(8);

    ck_assert_ptr_ne(list, NULL);
    ck_assert_uint_eq(list->nelts, 8);
    ck_assert_uint_eq(list->elts[0].tag, NULL_TAG);
    ck_assert_uint_eq(list->elts[0].value, 0x12345);
    free_simple_taglist(list);
    ck_assert_uint_eq(list->elts[1].tag, NULL_TAG);
    ck_assert_uint_eq(list->elts[1].value, 0xdeadbeef);

#test add_lookup
    simple_taglist *list = new_simple_taglist(1);
    simple_taglist *saved = list;
    unsigned *found;

    *add_simple_taglist(&list, 1) = 123;
    ck_assert_ptr_eq(list, saved);
    found = lookup_simple_taglist(&list, 1, false);
    ck_assert_ptr_ne(found, NULL);
    ck_assert_uint_eq(*found, 123);
    free_simple_taglist(list);

#test add_grow
    simple_taglist *list = new_simple_taglist(1);
    simple_taglist *saved = list;

    *add_simple_taglist(&list, 1) = 123;
    *add_simple_taglist(&list, 2) = 456;
    ck_assert_ptr_ne(list, saved);
    saved = list;
    *add_simple_taglist(&list, 3) = 789;
    ck_assert_ptr_eq(list, saved);

    free_simple_taglist(list);

#test lookup_add
    simple_taglist *list = new_simple_taglist(1);
    unsigned *found;

    found = lookup_simple_taglist(&list, 1, true);
    ck_assert_ptr_ne(found, NULL);
    ck_assert_uint_eq(*found, 0x12345);
    free_simple_taglist(list);


#test lookup
    simple_taglist *list = new_simple_taglist(4);
    unsigned *found;

    *add_simple_taglist(&list, 1) = 123;
    *add_simple_taglist(&list, 2) = 456;
    *add_simple_taglist(&list, 3) = 789;        
    found = lookup_simple_taglist(&list, 1, false);
    ck_assert_ptr_ne(found, NULL);
    ck_assert_uint_eq(*found, 123);
    found = lookup_simple_taglist(&list, 2, false);
    ck_assert_ptr_ne(found, NULL);
    ck_assert_uint_eq(*found, 456);
    found = lookup_simple_taglist(&list, 3, false);
    ck_assert_ptr_ne(found, NULL);
    ck_assert_uint_eq(*found, 789);
    found = lookup_simple_taglist(&list, 4, false);
    ck_assert_ptr_eq(found, NULL);
    free_simple_taglist(list);

#test lookupmulti
    simple_taglist *list = new_simple_taglist(4);
    unsigned *found;

    *add_simple_taglist(&list, 1) = 123;
    *add_simple_taglist(&list, 2) = 456;
    *add_simple_taglist(&list, 1) = 789;        
    found = lookup_simple_taglist(&list, 1, false);
    ck_assert_ptr_ne(found, NULL);
    ck_assert_uint_eq(*found, 123);
    free_simple_taglist(list);

#test add_delete
    simple_taglist *list = new_simple_taglist(1);
    unsigned *found;

    found = lookup_simple_taglist(&list, 1, true);
    ck_assert_ptr_ne(found, NULL);
    delete_simple_taglist(list, 1, false);
    found = lookup_simple_taglist(&list, 1, false);
    ck_assert_ptr_eq(found, NULL);
    free_simple_taglist(list);

#test add_delete_add
    simple_taglist *list = new_simple_taglist(1);
    simple_taglist *saved = list;
    unsigned *found;
    unsigned *added;

    found = lookup_simple_taglist(&list, 1, true);
    ck_assert_ptr_ne(found, NULL);
    *found = 123;
    delete_simple_taglist(list, 1, false);
    added = add_simple_taglist(&list, 2);
    ck_assert_ptr_eq(list, saved);
    ck_assert_ptr_eq(found, added);
    ck_assert_uint_eq(*added, 0x12345);
    free_simple_taglist(list);

#test add_add_delete_unhide
    simple_taglist *list = new_simple_taglist(2);
    unsigned *found;

    found = lookup_simple_taglist(&list, 1, true);
    *found = 123;
    found = add_simple_taglist(&list, 1);
    *found = 234;    
    found = lookup_simple_taglist(&list, 1, false);
    ck_assert_ptr_ne(found, NULL);
    ck_assert_uint_eq(*found, 123);
    delete_simple_taglist(list, 1, false);
    found = lookup_simple_taglist(&list, 1, false);
    ck_assert_ptr_ne(found, NULL);
    ck_assert_uint_eq(*found, 234);    
    free_simple_taglist(list);

#test add_add_delete_all
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
    ck_assert_ptr_eq(found, NULL);
    found = lookup_simple_taglist(&list, 2, false);    
    ck_assert_uint_eq(*found, 456);    
    free_simple_taglist(list);

#test add_add_foreach
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
    ck_assert_uint_eq(count, 2);
    ck_assert_uint_eq(sum, 357);

    free_simple_taglist(list);


#test get_none
    refcnt_taglist *list = new_refcnt_taglist(4);

    ck_assert_ptr_eq(get_refcnt_taglist(list, 1), NULL);
    free_refcnt_taglist(list);

#test put_get
    tagged_refcnt *obj = new_tagged_refcnt(0x12345);
    tagged_refcnt *got;
    refcnt_taglist *list = new_refcnt_taglist(4);

    got = put_refcnt_taglist(&list, 1, obj);
    ck_assert_ptr_eq(got, NULL);
    ck_assert_uint_eq(obj->refcnt, 2);
    got = get_refcnt_taglist(list, 1);
    ck_assert_ptr_eq(got, obj);
    ck_assert_uint_eq(obj->refcnt, 3);
    free_tagged_refcnt(got);    
    
    free_refcnt_taglist(list);
    ck_assert_uint_eq(obj->refcnt, 1);    
    free_tagged_refcnt(obj);

#test replace
    tagged_refcnt *obj = new_tagged_refcnt(0x12345);
    tagged_refcnt *obj1 = new_tagged_refcnt(0x12346);
    tagged_refcnt *got;
    refcnt_taglist *list = new_refcnt_taglist(4);

    replace_refcnt_taglist(&list, 1, obj);
    replace_refcnt_taglist(&list, 1, obj1);
    ck_assert_uint_eq(obj->refcnt, 1);
    got = get_refcnt_taglist(list, 1);
    ck_assert_ptr_eq(got, obj1);
    free_tagged_refcnt(got);    
    free_tagged_refcnt(obj1);    
    free_refcnt_taglist(list);
    free_tagged_refcnt(obj);


#test copy_refcnt
    tagged_refcnt *obj = new_tagged_refcnt(0x12345);
    refcnt_taglist *list = new_refcnt_taglist(4);
    refcnt_taglist *copy;

    replace_refcnt_taglist(&list, 1, obj);
    copy = copy_refcnt_taglist(list);
    ck_assert_ptr_ne(copy, NULL);
    ck_assert_uint_eq(copy->nelts, list->nelts);
    ck_assert_ptr_eq(copy->elts[0].value, obj);
    ck_assert_uint_eq(obj->refcnt, 3);    
    free_refcnt_taglist(list);
    free_refcnt_taglist(copy);
    free_tagged_refcnt(obj);
