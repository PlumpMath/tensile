/*!= Generic allocator routines
 * (c) 2015-2016  Artem V. Andreev
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
 */

/* HEADER */
#include "compiler.h"
/* END */
#include "allocator_common.h"

/* parameter */
#define ALLOC_TYPE /* required */
/* parameter */
#define ALLOC_NAME ALLOC_TYPE

/* parameter */
#define TYPE_IS_REFCNTED false

/* parameter */
#define TRACK_ALLOCATOR false

/* parameter */
#define ALLOC_CONSTRUCTOR_ARGS void

/* parameter */
#define ALLOC_CONSTRUCTOR_CODE(_obj) {}
/* parameter */
#define ALLOC_DESTRUCTOR_CODE(_obj) {}
/* parameter */
#define ALLOC_COPY_CODE(_obj) {}

/* local */
#define ALLOC_PREFIX(_name) QNAME(_name, ALLOC_NAME)

/* local */
#define freelists_TYPE ALLOC_PREFIX(freelists)

/* local */
#define tracks_TYPE ALLOC_PREFIX(tracks)


/* parameter */
#define USE_ALLOC_POOL false
/* parameter */
#define ALLOC_POOL_PTR NULL
/* parameter */
#define ALLOC_POOL_SIZE NULL
/* parameter */
#define ALLOC_POOL_ALIGN_AS double

/* local */
#define freelist_TYPE ALLOC_PREFIX(freelist)
/* local */
#define track_TYPE ALLOC_PREFIX(track)

static freelist_t *freelist_TYPE;
#if TRACK_ALLOCATOR
static size_t track_TYPE;
#endif

/* TESTS */
#include "assertions.h"
#define TESTSUITE "Allocator"

enum object_state {
    STATE_INITIALIZED = STATIC_ARBITRARY(16),
    STATE_COPIED = STATIC_ARBITRARY(16),
    STATE_FINALIZED = STATIC_ARBITRARY(16)
};

typedef struct simple_type {
    void *placeholder;
    testval_tag_t tag;
    enum object_state state;
} simple_type;

/* instantiate */
#define ALLOC_TYPE simple_type
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_ARGS testval_tag_t tag
#define ALLOC_CONSTRUCTOR_CODE(_obj)            \
    do {                                        \
        (_obj)->tag = tag;                      \
        (_obj)->state = STATE_INITIALIZED;      \
    } while(0)
#define ALLOC_COPY_CODE(_obj) (_obj)->state = STATE_COPIED
#define ALLOC_DESTRUCTOR_CODE(_obj) (_obj)->state = STATE_FINALIZED
#include "allocator_api.h"
#include "allocator_impl.c"
/* end */

/* END */

/* local */
#define alloc_TYPE ALLOC_PREFIX(alloc)

static returns(not_null) returns(fresh_pointer) returns(important)
ALLOC_TYPE *alloc_TYPE(void)
{
    ALLOC_TYPE *obj;

#if TRACK_ALLOCATOR
    track_TYPE++;
#endif

#if USE_ALLOC_POOL
    assert(ALLOC_POOL_PTR != NULL);

    obj = pool_alloc(&ALLOC_POOL_PTR, &ALLOC_POOL_SIZE,
                     sizeof(*obj), sizeof(ALLOC_POOL_ALIGN_AS));
    if (obj != NULL)
    {
        return obj;
    }
#endif

    if (freelist_TYPE != NULL)
    {
        obj = (ALLOC_TYPE *)freelist_TYPE;
        freelist_TYPE = freelist_TYPE->chain;

        return obj;
    }

    return frlmalloc(sizeof(*obj));
}

/* local */
#define new_TYPE ALLOC_PREFIX(new)

/* public */
returns(not_null) returns(important)
ALLOC_TYPE *new_TYPE(ALLOC_CONSTRUCTOR_ARGS)
{
    ALLOC_TYPE *_obj = alloc_TYPE();

#if TYPE_IS_REFCNTED
    _obj->refcnt = 1;
#endif
    ALLOC_CONSTRUCTOR_CODE(_obj);
    return _obj;
}

/*! Test: Allocate an object
 */
static void allocate(testval_tag_t tag)
{
    simple_type *st = new_simple_type(tag);

    ASSERT_NOT_NULL(st);
    ASSERT_EQ(bits, st->tag, tag);
    ASSERT_EQ(unsigned, track_simple_type, 1);
    free_simple_type(st);
}

/*! Test: Allocate and free an object
 */
static void allocate_and_free(testval_tag_t tag)
{
    simple_type *st = new_simple_type(tag);

    free_simple_type(st);
    ASSERT_EQ(ptr, freelist_simple_type, (void *)st);
    ASSERT_EQ(unsigned, st->state, STATE_FINALIZED);
    ASSERT_EQ(unsigned, track_simple_type, 0);
}

/*! Test: Allocate and free an object and allocate another
 */
static void allocate_free_allocate(testval_tag_t tag)
{
    simple_type *st = new_simple_type(tag);
    simple_type *st1;

    free_simple_type(st);
    st1 = new_simple_type(~tag);
    ASSERT_EQ(ptr, st, st1);
    ASSERT_NULL(freelist_simple_type);
    ASSERT_EQ(unsigned, st1->tag, ~tag);
    ASSERT_EQ(unsigned, track_simple_type, 1);
    free_simple_type(st1);
}

/* local */
#define unshare_TYPE ALLOC_PREFIX(unshare)

/* public */
static inline arguments(not_null)
void unshare_TYPE(unused ALLOC_TYPE *_obj)
{
#if TYPE_IS_REFCNTED
    _obj->refcnt = 1;
#endif
    ALLOC_COPY_CODE(_obj);
}

/* local */
#define copy_TYPE ALLOC_PREFIX(copy)

/* public */
returns(not_null) returns(important) arguments(not_null)
ALLOC_TYPE *copy_TYPE(const ALLOC_TYPE *_orig)
{
    ALLOC_TYPE *_copy = alloc_TYPE();

    assert(_orig != NULL);
    memcpy(_copy, _orig, sizeof(*_copy));
    unshare_TYPE(_copy);
    return _copy;
}

/*! Test: Copy
 */
static void test_copy(testval_tag_t tag)
{
    simple_type *st = new_simple_type(tag);
    simple_type *st1;

    st1 = copy_simple_type(st);
    ASSERT_NEQ(ptr, st, st1);
    ASSERT_EQ(bits, st1->tag, st->tag);
    ASSERT_EQ(unsigned, st1->state, STATE_COPIED);
    ASSERT_EQ(unsigned, track_simple_type, 2);
    free_simple_type(st);
    free_simple_type(st1);
    ASSERT_EQ(unsigned, track_simple_type, 0);
}

/*! Test: Deallocate and copy
 */
static void deallocate_and_copy(testval_tag_t tag)
{
    simple_type *st = new_simple_type(tag);
    simple_type *st1 = new_simple_type(~tag);
    simple_type *st2;

    free_simple_type(st);
    st2 = copy_simple_type(st1);
    ASSERT_EQ(ptr, st2, st);
    free_simple_type(st1);
    free_simple_type(st2);
    ASSERT_EQ(unsigned, track_simple_type, 0);
}

/* local */
#define free_TYPE ALLOC_PREFIX(free)

/* public */
void free_TYPE(ALLOC_TYPE *_obj)
{
    if (_obj == NULL) {
        return;
    }

#if TYPE_IS_REFCNTED
    assert(_obj->refcnt > 0);
    if (--_obj->refcnt > 0)
    {
        return;
    }
#endif
    ALLOC_DESTRUCTOR_CODE(_obj);
#if TRACK_ALLOCATOR
    assert(track_TYPE > 0);
    track_TYPE--;
#endif
#if USE_ALLOC_POOL
    if (pool_maybe_free(&ALLOC_POOL_PTR, &ALLOC_POOL_SIZE,
                        _obj, sizeof(*_obj),
                        sizeof(ALLOC_POOL_ALIGN_AS)))
    {
        return;
    }
#endif
    ((freelist_t *)_obj)->chain = freelist_TYPE;
    freelist_TYPE = (freelist_t *)_obj;
}

/*! Test: Allocate and free an object and allocate two more
 */
static void allocate_free_allocate2(testval_tag_t tag)
{
    simple_type *st = new_simple_type(tag);
    simple_type *st1;
    simple_type *st2;

    free_simple_type(st);
    st1 = new_simple_type(~tag);
    ASSERT_EQ(ptr, st, st1);
    st2 = new_simple_type(tag + 1);
    ASSERT_NEQ(ptr, st1, st2);
    ASSERT_EQ(unsigned, track_simple_type, 2);
    free_simple_type(st1);
    free_simple_type(st2);
    ASSERT_EQ(unsigned, track_simple_type, 0);
}

/*! Test: Free NULL
 */
static void free_null(void)
{
    freelist_t *prev = freelist_simple_type;
    free_simple_type(NULL);
    ASSERT_EQ(ptr, freelist_simple_type, prev);
    ASSERT_EQ(unsigned, track_simple_type, 0);
}

/* TESTS */

/* instantiate */
#define ALLOC_TYPE short
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_CODE(_obj) *(_obj) = (short)STATE_INITIALIZED
#include "allocator_api.h"
#include "allocator_impl.c"
/* end */

/*! Test: Allocate and free a small object
 */
static void alloc_small(void)
{
    short *sm;
    short *sm1;

    sm = new_short();
    ASSERT_EQ(int, *sm, (int)STATE_INITIALIZED);
    free_short(sm);
    sm1 = new_short();
    ASSERT_EQ(ptr, sm, sm1);
    ASSERT_EQ(unsigned, track_short, 1);
    free_short(sm1);
    ASSERT_EQ(unsigned, track_short, 0);
    ASSERT_EQ(ptr, freelist_short, sm1);
}


static void *shared_pool_ptr;
static size_t shared_pool_size;

/* instantiate */
#define ALLOC_TYPE simple_type
#define ALLOC_NAME pool_simple_type
#define USE_ALLOC_POOL true
#define ALLOC_POOL_PTR shared_pool_ptr
#define ALLOC_POOL_SIZE shared_pool_size
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_ARGS testval_tag_t tag
#define ALLOC_CONSTRUCTOR_CODE(_obj)            \
    do {                                        \
        (_obj)->tag = tag;                      \
        (_obj)->state = STATE_INITIALIZED;      \
    } while(0)
#define ALLOC_COPY_CODE(_obj) (_obj)->state = STATE_COPIED
#define ALLOC_DESTRUCTOR_CODE(_obj) (_obj)->state = STATE_FINALIZED
#include "allocator_api.h"
#include "allocator_impl.c"
/* end */

/*! Test: Allocate from pool
 */
static void alloc_from_pool(testval_tag_t tag)
{
    simple_type *st;
    void *pool_base;

    pool_base = shared_pool_ptr = malloc(sizeof(simple_type));
    shared_pool_size = sizeof(simple_type);

    st = new_pool_simple_type(tag);
    ASSERT_NOT_NULL(st);
    ASSERT_EQ(unsigned, st->state, STATE_INITIALIZED);
    ASSERT_EQ(unsigned, st->tag, tag);
    ASSERT_EQ(ptr, st, pool_base);
    ASSERT_EQ(unsigned, shared_pool_size, 0);
    ASSERT_EQ(ptr, (uint8_t *)shared_pool_ptr,
              (uint8_t *)pool_base + sizeof(simple_type));
    free_pool_simple_type(st);
    ASSERT_EQ(ptr, shared_pool_ptr, pool_base);
    ASSERT_EQ(unsigned, shared_pool_size, sizeof(simple_type));
    ASSERT_NULL(freelist_pool_simple_type);
    free(pool_base);
}

/*! Test: Allocate from pool and then malloc
 */
static void alloc_from_pool_and_malloc(testval_tag_t tag)
{
    simple_type *st;
    simple_type *st1;
    void *pool_base;

    pool_base = shared_pool_ptr = malloc(sizeof(simple_type));
    shared_pool_size = sizeof(simple_type);

    st = new_pool_simple_type(tag);
    st1 = new_pool_simple_type(~tag);
    ASSERT_NEQ(ptr, st, st1);
    ASSERT_EQ(unsigned, shared_pool_size, 0);
    ASSERT_NEQ(ptr, st1, (uint8_t *)pool_base + sizeof(simple_type));
    ASSERT_EQ(ptr, shared_pool_ptr, (uint8_t *)pool_base + sizeof(simple_type));
    free_pool_simple_type(st1);
    free(pool_base);
}

/*! Test: Allocate from pool and free in reverse order
 */
static void alloc_from_pool_and_reverse_free(testval_tag_t tag)
{
    simple_type *st;
    simple_type *st1;
    void *pool_base;
    static const size_t pool_item_size =
        (sizeof(simple_type) + sizeof(double) - 1) / sizeof(double) *
        sizeof(double);

    shared_pool_size = 2 * pool_item_size;
    pool_base = shared_pool_ptr = malloc(shared_pool_size);

    st = new_pool_simple_type(tag);
    st1 = new_pool_simple_type(~tag);
    ASSERT_EQ(ptr, st, pool_base);
    ASSERT_EQ(ptr, (uint8_t *)st1, (uint8_t *)st + pool_item_size);
    ASSERT_EQ(unsigned, shared_pool_size, 0);
    free_pool_simple_type(st);
    ASSERT_EQ(unsigned, shared_pool_size, 0);
    ASSERT_EQ(ptr, shared_pool_ptr,
              (uint8_t *)pool_base + 2 * pool_item_size);
    free_pool_simple_type(st1);
    ASSERT_EQ(ptr, freelist_pool_simple_type, st);
    ASSERT_EQ(ptr, shared_pool_ptr,
              (uint8_t *)pool_base + pool_item_size);    
    /* no call to free(pool_base) as the memory remains in the freelist */
    shared_pool_ptr = NULL;
}

/* instantiate */
#define ALLOC_TYPE short
#define ALLOC_NAME pool_short
#define USE_ALLOC_POOL true
#define ALLOC_POOL_PTR shared_pool_ptr
#define ALLOC_POOL_SIZE shared_pool_size
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_ARGS short val
#define ALLOC_CONSTRUCTOR_CODE(_obj) *(_obj) = val
#include "allocator_api.h"
#include "allocator_impl.c"
/* end */

/*! Test: Allocate pool from with alignment
 */
static void alloc_from_pool_align(testval_tag_t tag)
{
    short *shrt;
    short *shrt2;
    void *pool_base;

    pool_base = shared_pool_ptr = malloc(sizeof(double) + sizeof(short));
    shared_pool_size = sizeof(double) + sizeof(short);

    shrt = new_pool_short((short)tag);
    ASSERT_EQ(ptr, shrt, pool_base);
    ASSERT_EQ(ptr, shared_pool_ptr, (uint8_t *)pool_base + sizeof(double));
    ASSERT_EQ(unsigned, shared_pool_size, sizeof(short));
    ASSERT_EQ(int, *shrt, (short)tag);
    
    shrt2 = new_pool_short((short)~tag);
    ASSERT_NEQ(ptr, shrt2, shrt + 1);
    ASSERT_EQ(ptr, shared_pool_ptr, (uint8_t *)pool_base + sizeof(double));
    ASSERT_EQ(unsigned, shared_pool_size, sizeof(short));
    ASSERT_EQ(int, *shrt2, (short)~tag);
    free_pool_short(shrt2);
    /* shrt will be freed with the pool */
    free(pool_base);
}
    
/* END */

/* public */
#if TYPE_IS_REFCNTED

/* local */
#define use_TYPE ALLOC_PREFIX(use)

/* public */
static inline
ALLOC_TYPE *use_TYPE(ALLOC_TYPE *val)
{
    if (val == NULL)
    {
        return NULL;
    }
    val->refcnt++;
    return val;
}

/* local */
#define maybe_copy_TYPE ALLOC_PREFIX(maybe_copy)

static inline returns(important)
ALLOC_TYPE *maybe_copy_TYPE(ALLOC_TYPE *val)
{
    if (val == NULL || val->refcnt == 1)
    {
        return val;
    }
    else
    {
        ALLOC_TYPE *result = copy_TYPE(val);

        free_TYPE(val);
        return result;
    }
}

/* local */
#define assign_TYPE ALLOC_PREFIX(assign)

/* public */
static inline argument(not_null, 1)
void assign_TYPE(ALLOC_TYPE **loc, ALLOC_TYPE *val)
{
    assert(loc != NULL);
    use_TYPE(val);
    free_TYPE(*loc);
    *loc = val;
}

/* local */
#define move_TYPE ALLOC_PREFIX(move)

/* public */
static inline argument(not_null, 1)
void move_TYPE(ALLOC_TYPE **loc, ALLOC_TYPE *val)
{
    assign_TYPE(loc, val);
    free_TYPE(val);
}

/* TESTS */
typedef struct refcnt_type {
    void *placeholder;
    unsigned refcnt;
    testval_tag_t tag;
    enum object_state state;
} refcnt_type;

/* instantiate */
#define TYPE_IS_REFCNTED true
#define ALLOC_TYPE refcnt_type
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_ARGS testval_tag_t tag
#define ALLOC_CONSTRUCTOR_CODE(_obj)            \
    do {                                        \
        (_obj)->tag = tag;                      \
        (_obj)->state = STATE_INITIALIZED;      \
    } while(0)
#define ALLOC_COPY_CODE(_obj) (_obj)->state = STATE_COPIED
#define ALLOC_DESTRUCTOR_CODE(_obj) (_obj)->state = STATE_FINALIZED
#include "allocator_api.h"
#include "allocator_impl.c"
/* end */

/*! Test: Allocate refcounted
 */
static void allocate_refcnted(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);

    ASSERT_NOT_NULL(rt);
    ASSERT_EQ(unsigned, rt->tag, tag);
    ASSERT_EQ(unsigned, rt->state, STATE_INITIALIZED);
    ASSERT_EQ(unsigned, rt->refcnt, 1);
    ASSERT_EQ(unsigned, track_refcnt_type, 1);
    free_refcnt_type(rt);
}

/*! Test: Allocate and free refcounted
 */
static void allocate_free_refcnted(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);

    free_refcnt_type(rt);
    ASSERT_EQ(unsigned, rt->state, STATE_FINALIZED);
    ASSERT_EQ(unsigned, rt->refcnt, 0);
    ASSERT_EQ(unsigned, track_refcnt_type, 0);
    ASSERT_EQ(ptr, freelist_refcnt_type, rt);
}

/*! Test: Allocate, free and reallocate refcounted
 */
static void allocate_free_allocate2_refcnted(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);
    refcnt_type *rt1;

    free_refcnt_type(rt);
    rt1 = new_refcnt_type(~tag);

    ASSERT_EQ(ptr, rt, rt1);
    ASSERT_EQ(unsigned, rt1->refcnt, 1);
    ASSERT_EQ(unsigned, rt1->tag, ~tag);
    free_refcnt_type(rt1);
    ASSERT_EQ(unsigned, rt1->state, STATE_FINALIZED);
}

/*! Test: Allocate, use and free refcounted
 */
static void allocate_use_free_refcnt(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);
    refcnt_type *use = use_refcnt_type(rt);

    ASSERT_EQ(ptr, use, rt);
    ASSERT_EQ(unsigned, use->refcnt, 2);
    free_refcnt_type(rt);
    ASSERT_EQ(unsigned, use->refcnt, 1);
    ASSERT_EQ(unsigned, use->state, STATE_INITIALIZED);
    free_refcnt_type(use);
    ASSERT_EQ(unsigned, use->state, STATE_FINALIZED);
}

/*! Test: Copy refcounted
 */
static void copy_refcnted(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);
    refcnt_type *rt1 = copy_refcnt_type(rt);

    ASSERT_NEQ(ptr, rt, rt1);
    ASSERT_EQ(unsigned, rt->refcnt, 1);
    ASSERT_EQ(unsigned, rt1->refcnt, 1);
    ASSERT_EQ(unsigned, rt1->state, STATE_COPIED);
    free_refcnt_type(rt1);
    free_refcnt_type(rt);
    ASSERT_EQ(unsigned, track_refcnt_type, 0);
}

/*! Test: Assign to NULL
 */
static void assign_to_null(testval_tag_t tag)
{
    refcnt_type *location = NULL;
    refcnt_type *rt = new_refcnt_type(tag);

    assign_refcnt_type(&location, rt);
    ASSERT_EQ(ptr, location, rt);
    ASSERT_EQ(unsigned, rt->refcnt, 2);
    free_refcnt_type(location);
    free_refcnt_type(rt);
    ASSERT_EQ(unsigned, track_refcnt_type, 0);
}

/*! Test: Assign to itself
 */
static void assign_to_self(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);
    refcnt_type *location = rt;

    assign_refcnt_type(&location, rt);
    ASSERT_EQ(ptr, location, rt);
    ASSERT_EQ(unsigned, rt->refcnt, 1);
    ASSERT_NEQ(unsigned, rt->state, STATE_FINALIZED);
    free_refcnt_type(rt);
    ASSERT_EQ(unsigned, rt->state, STATE_FINALIZED);
    ASSERT_EQ(unsigned, track_refcnt_type, 0);
}

/*! Test: Assign NULL
 */
static void assign_null(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);
    refcnt_type *location = rt;
    assign_refcnt_type(&location, NULL);
    ASSERT_NULL(location);
    ASSERT_EQ(unsigned, rt->state, STATE_FINALIZED);
    ASSERT_EQ(unsigned, track_refcnt_type, 0);
}

/*! Test: Maybe copy
 */
static void maybe_copy(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);
    refcnt_type *rtc = maybe_copy_refcnt_type(rt);

    ASSERT_EQ(ptr, rt, rtc);
    ASSERT_EQ(unsigned, rt->refcnt, 1);

    use_refcnt_type(rt);
    rtc = maybe_copy_refcnt_type(rt);
    ASSERT_NEQ(ptr, rt, rtc);
    ASSERT_EQ(unsigned, rt->refcnt, 1);
    ASSERT_EQ(unsigned, rtc->state, STATE_COPIED);
    free_refcnt_type(rtc);
    free_refcnt_type(rt);
    ASSERT_EQ(unsigned, track_refcnt_type, 0);
}

/*! Test: Move
 */
static void move_refcnt(testval_tag_t tag)
{
    refcnt_type *rt = new_refcnt_type(tag);
    refcnt_type *rt1 = new_refcnt_type(~tag);

    move_refcnt_type(&rt1, rt);
    ASSERT_EQ(ptr, rt1, rt);
    ASSERT_EQ(unsigned, rt1->refcnt, 1);
    ASSERT_EQ(unsigned, rt1->state, STATE_INITIALIZED);
    ASSERT_EQ(unsigned, rt1->tag, tag);
    free_refcnt_type(rt1);
    ASSERT_EQ(unsigned, track_refcnt_type, 0);
}

/* END */

#endif /* TYPE_IS_REFCNTED */
