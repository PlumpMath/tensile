/*!= Generic array allocator routines
 * (c) 2016  Artem V. Andreev
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
#define TRACK_ALLOCATOR false

/* parameter */
#define ALLOC_CONSTRUCTOR_CODE(_obj) {}
/* parameter */
#define ALLOC_DESTRUCTOR_CODE(_obj) {}
/* parameter */
#define ALLOC_COPY_CODE(_obj) {}

/* local */
#define ALLOC_PREFIX(_name) QNAME(_name, ALLOC_NAME)

/* parameter */
#define USE_ALLOC_POOL false
/* parameter */
#define ALLOC_POOL_PTR NULL
/* parameter */
#define ALLOC_POOL_SIZE NULL

/* parameter */
#define MAX_BUCKET /* required */

/* parameter */
#define BUCKET_SIZE 1

/* parameter */
#define BUCKET_LOG2_SCALE false

/* local */
#define freelist_TYPE ALLOC_PREFIX(freelist)
/* local */
#define track_TYPE ALLOC_PREFIX(track)

static freelist_t *freelist_TYPE[MAX_BUCKET];
#if TRACK_ALLOCATOR
static size_t track_TYPE[MAX_BUCKET];
#endif

/* TESTS */
#include "assertions.h"
#define TESTSUITE "Array allocator"

enum object_state {
    STATE_INITIALIZED = STATIC_ARBITRARY(16),
    STATE_COPIED = STATIC_ARBITRARY(16),
    STATE_FINALIZED = STATIC_ARBITRARY(16)
};

typedef struct simple_type {
    enum object_state state;
} simple_type;

/* instantiate */
#define ALLOC_TYPE simple_type
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_CODE(_obj)            \
    (_obj)->state = STATE_INITIALIZED
#define ALLOC_COPY_CODE(_obj) (_obj)->state = STATE_COPIED
#define ALLOC_DESTRUCTOR_CODE(_obj) (_obj)->state = STATE_FINALIZED
#include "array_api.h"
#include "array_impl.c"
/* end */

/* END */

/* local */
#define alloc_TYPE ALLOC_PREFIX(alloc)

static returns(not_null) returns(fresh_pointer) returns(important)
ALLOC_TYPE *alloc_TYPE(void)
{
    ALLOC_TYPE *obj;
#if USE_ALLOC_POOL
    static const size_t pool_incr = (sizeof(*obj) + sizeof(double) - 1) /
        sizeof(double) * sizeof(double);
#endif

#if TRACK_ALLOCATOR
    track_TYPE++;
#endif

#if USE_ALLOC_POOL
    assert(ALLOC_POOL_PTR != NULL);
    if (ALLOC_POOL_SIZE >= pool_incr)
    {
        obj = (ALLOC_TYPE *)ALLOC_POOL_PTR;
        ALLOC_POOL_PTR = (uint8_t *)ALLOC_POOL_PTR + pool_incr;
        ALLOC_POOL_SIZE -= pool_incr;
        PROBE(alloc_from_pool);

        return obj;
    }
#endif

    if (freelist_TYPE != NULL)
    {
        PROBE(alloc_from_freelist);
        obj = (ALLOC_TYPE *)freelist_TYPE;
        freelist_TYPE = freelist_TYPE->chain;

        return obj;
    }

    PROBE(alloc_malloc);
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
    PROBE(init_refcnt);
    _obj->refcnt = 1;
#endif
    PROBE(initialize_object);
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
    PROBE(unshare_refcnt);
    _obj->refcnt = 1;
#endif
    PROBE(unshare_object);
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
    PROBE(do_copy);
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
        PROBE(free_null);
        return;
    }

#if TYPE_IS_REFCNTED
    assert(_obj->refcnt > 0);
    if (--_obj->refcnt > 0)
    {
        PROBE(decr_refcnt);
        return;
    }
#endif
    ALLOC_DESTRUCTOR_CODE(_obj);
#if TRACK_ALLOCATOR
    assert(track_TYPE > 0);
    track_TYPE--;
#endif
#if USE_ALLOC_POOL
    if ((uint8_t *)_obj + sizeof(*_obj) == ALLOC_POOL_PTR)
    {
        PROBE(free_to_pool);
        ALLOC_POOL_PTR = (void *)_obj;
        ALLOC_POOL_SIZE += sizeof(*_obj);
        return;
    }
#endif
    PROBE(free_to_list);
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
#define REQUIRE_SINGLE_ALLOCATOR true
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
#define REQUIRE_SINGLE_ALLOCATOR true
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
        PROBE(use_null);
        return NULL;
    }
    val->refcnt++;
    PROBE(use_normal);
    return val;
}

/* local */
#define maybe_copy_TYPE ALLOC_PREFIX(maybe_copy)

static inline
ALLOC_TYPE *maybe_copy_TYPE(ALLOC_TYPE *val)
{
    if (val == NULL || val->refcnt == 1)
    {
        PROBE(maybe_copy_nocopy);
        return val;
    }
    else
    {
        ALLOC_TYPE *result = copy_TYPE(val);
        
        PROBE(maybe_copy_copy);
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
    PROBE(assign);
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
    PROBE(move);
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
#define REQUIRE_SINGLE_ALLOCATOR true
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
/* END */

#endif /* TYPE_IS_REFCNTED */


#endif /* REQUIRE_SINGLE_ALLOCATOR */

#if 0

/*@<Plain allocator declarations@>+=*/
extern NONNULL(1) void allocator_prealloc(allocator_t *alloc,
                                                size_t n);

/*@<Plain allocator@>+=*/
void allocator_prealloc(allocator_t *alloc, size_t n)
{
    size_t i;
    void *objs = malloc(n * alloc->object_size);
    void *next;
    void *iter;

    assert(alloc->object_size >= sizeof(freelist_t));
    for (i = 0, iter = objs; i < n - 1; i++, iter = next)
    {
        next = (uint8_t *)iter + alloc->object_size;
        ((freelist_t *)iter)->chain = next;
    }
    ((freelist_t *)&objs[size - 1])->chain = alloc->free_list;
    alloc->free_list = objs;
}

/**
 * Generates declarations for freelist-based typed memory allocations
 * which are reference-counted
 * - see DECLARE_TYPE_ALLOCATOR()
 * - `_type *use_<_type>(_type *)`
 * - `void assign_<_type>(_type **, _type *)`
 * @param _scope Scope for declared functions
 * @param _type  The type of allocated objects
 * @param _args  Arguments to the constructor in parentheses
 */
#define DECLARE_REFCNT_ALLOCATOR(_scope, _type, _args)                  \
    DECLARE_TYPE_ALLOCATOR(_scope, _type, _args);                       \
                                                                        \
    static inline _type *use_##_type(_type *val)                        \
    {                                                                   \
        if (!val) return NULL;                                          \
        val->refcnt++;                                                  \
        return val;                                                     \
    }                                                                   \
                                                                        \
    NONNULL(1)                                                          \
    static inline void assign_##_type(_type **loc, _type *val)          \
    {                                                                   \
        use_##_type(val);                                               \
        free_##_type(*loc);                                             \
        *loc = val;                                                     \
    }                                                                   \
    struct fake


#define REFCNT_REQUIRED_FIELDS unsigned refcnt


/**
 * Generates a declaration for a function to pre-allocate a free-list
 * - `void preallocate_<_type>s(size_t size)`
 */
#define DECLARE_TYPE_PREALLOC(_scope, _type)                            \
    GENERATED_DECL_##_scope void preallocate_##_type##s(size_t size)

/**
 * Generate declarations for free-list based array allocations
 */
#define DECLARE_ARRAY_ALLOCATOR(_scope, _name, _type)                   \
    GENERATED_DECL_##_scope _type *new_##_name(size_t n)                \
    WARN_UNUSED_RESULT RETURNS_NONNULL;                                 \
    GENERATED_DECL_##_scope void free_##_name(_type *_obj, size_t n);   \
    GENERATED_DECL_##_scope _type *unshare_##_name(_type *_oldobj,      \
                                                   size_t n)            \
    NONNULL_ALL RETURNS_NONNULL;                                        \
    GENERATED_DECL_##_scope _type *copy_##_name(const _type *_oldobj,   \
                                                size_t n)               \
    NONNULL_ALL RETURNS_NONNULL;                                        \
                                                                        \
    GENERATED_DECL_##_scope _type *resize_##_name(_type **arr,          \
                                                  size_t *oldn,         \
                                                  size_t newn)          \
    NONNULL_ALL WARN_UNUSED_RESULT;                                     \
                                                                        \
    NONNULL WARN_UNUSED_RESULT                                          \
    static inline _type *ensure_##_name##_size(_type **_arr,            \
                                               size_t *oldn,            \
                                               size_t req,              \
                                               size_t delta)            \
    {                                                                   \
        if (*oldn >= req)                                               \
            return *_arr;                                               \
        return resize_##_name(_arr, oldn, req + delta);                 \
    }                                                                   \
    struct fake

/**
 * Generate declarations for arrays of refcounter objects
 */

#define DECLARE_ARRAY_ALLOCATOR_REFCNT(_scope, _name, _type)            \
    DECLARE_ARRAY_ALLOCATOR(_scope, _name, _type *);                    \
                                                                        \
    NONNULL(1)                                                          \
    static inline void assign_##_name(_type **base, size_t idx, _type *val) \
    {                                                                   \
        assign_##_type(base + idx, val);                                \
    }                                                                   \
    struct fake



/**
 * Generate declarations for free-list based array allocations
 * which are reference-counted
 *
 * @test Background:
 * @code
 * DECLARE_ARRAY_TYPE(refcnt_array,
 * void *ptr; unsigned refcnt; unsigned tag;,
 * unsigned);
 *
 * DECLARE_REFCNT_ARRAY_ALLOCATOR(refcnt_array);
 * DEFINE_REFCNT_ARRAY_ALLOCATOR(refcnt_array,
 * linear, 4, arr, i,
 * {arr->tag = STATE_INITIALIZED;},
 * {}, {}, {}, {}, {}, {},
 * {arr->tag = STATE_FINALIZED;}, {});
 * @endcode
 *
 * @test
 * Allocate and free refcounted array
 * - Given a fresh array
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * refcnt_array *rt = new_refcnt_array(sz);
 * @endcode
 * - Verify that it is allocated `ASSERT_PTR_NEQ(rt, NULL);`
 * - Verify that it is initialized
 *   `ASSERT_UINT_EQ(rt->tag, STATE_INITIALIZED);`
 * - Verify that the number of elements is correct
 *   `ASSERT_UINT_EQ(rt->nelts, sz);`
 * - Verify that the refcounter is 1
 *   `ASSERT_UINT_EQ(rt->refcnt, 1);`
 * - Cleanup: `free_refcnt_array(rt);`
 *
 * @test
 * Allocate, use and free refcounted array
 * - Given a fresh array
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * refcnt_array *rt = new_refcnt_array(sz);
 * @endcode
 * - When it is used
 * `refcnt_array *use = use_refcnt_array(rt);`
 * - Then the pointer does not change
 * `ASSERT_PTR_EQ(use, rt);`
 * - And the refcounter is incremented
 * `ASSERT_UINT_EQ(use->refcnt, 2);`
 * - When it is freed:
 * `free_refcnt_array(rt);`
 * - Then the refcounter is decremented
 * `ASSERT_UINT_EQ(use->refcnt, 1);`
 * - But the object is not finalized:
 * `ASSERT_UINT_EQ(use->tag, STATE_INITIALIZED);`
 * - When it is freed again:
 * `free_refcnt_array(use);`
 * - Then it is finalized:
 * `ASSERT_UINT_EQ(use->tag, STATE_FINALIZED);`
 *
 * @test
 * Copy a refcounted array
 * - Given a new refcounted array:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * refcnt_array *rt = new_refcnt_array(sz);
 * @endcode
 * - When it is copied:
 * `refcnt_array *rt1 = copy_refcnt_array(rt);`
 * - Then the memory is not shared `ASSERT_PTR_NEQ(rt, rt1);`
 * - And the refcounter is not changed: `ASSERT_UINT_EQ(rt->refcnt, 1);`
 * - And the refcounter of the copy is 1:
 * `ASSERT_UINT_EQ(rt1->refcnt, 1);`
 * - And the copy is initialized:
 * `ASSERT_UINT_EQ(rt1->tag, STATE_INITIALIZED);`
 * - And the number of elements of the copy is correct
 * `ASSERT_UINT_EQ(rt1->nelts, sz);`
 * - Cleanup:
 * @code
 * free_refcnt_array(rt1);
 * free_refcnt_array(rt);
 * @endcode
 */

#if defined(IMPLEMENT_ALLOCATOR) || defined(__DOXYGEN__)


/** @cond DEV */
/**
 * Generates definitions for allocator functions declared per
 * DECLARE_TYPE_ALLOCATOR() and family
 *
 */
#define DEFINE_TYPE_ALLOC_COMMON(_scope, _type, _args, _init,           \
                                 _clone,                                \
                                 _destructor, _fini)                    \
    ALLOC_COUNTER(_type);                                               \
    static freelist_t *freelist_##_type;                                \
                                                                        \
    MALLOC_LIKE ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL            \
    static _type *alloc_##_type(void)                                   \
    {                                                                   \
        _type *_var;                                                    \
                                                                        \
        if (freelist_##_type == NULL)                                   \
            _var = frlmalloc(sizeof(*_var));                            \
        else                                                            \
        {                                                               \
            _var = (_type *)freelist_##_type;                           \
            freelist_##_type = freelist_##_type->chain;                 \
        }                                                               \
        TRACK_ALLOC(_type);                                             \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    GENERATED_DEF_##_scope _type *new_##_type _args                     \
    {                                                                   \
        _type *_OBJHOOK_VARNAME = alloc_##_type();                      \
        _init;                                                          \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    GENERATED_DEF_##_scope _type *copy_##_type(const _type *_var)       \
    {                                                                   \
        _type *_result = alloc_##_type();                               \
                                                                        \
        assert(_var != NULL);                                           \
        memcpy(_result, _var, sizeof(_type));                           \
        _clone(_result);                                                \
        return _result;                                                 \
    }                                                                   \
                                                                        \
    _destructor(_type *_var)                                            \
    {                                                                   \
        if (!_var) return;                                              \
        _fini(_var);                                                    \
        ((freelist_t *)_var)->chain = freelist_##_type;                 \
        freelist_##_type = (freelist_t *)_var;                          \
        TRACK_FREE(_type);                                              \
    }                                                                   \
    struct fake

/** @endcond */


/**
 * Generates definitions for allocator functions declared per
 * DECLARE_TYPE_ALLOCATOR()
 */
#define DEFINE_TYPE_ALLOCATOR(_scope, _type, _args, _init, _clone, _fini) \
    DEFINE_TYPE_ALLOC_COMMON(_scope, _type, _args, _init, _clone,       \
                             GENERATED_DEF_##_scope void free_##_type, _fini)

/** @cond DEV */
/**
 * Generates a reference-counting `free` function
 */
#define DEFINE_REFCNT_FREE(_scope, _type)                               \
    GENERATED_DEF_##_scope void free_##_type(_type *_var)               \
    {                                                                   \
        if (!_var) return;                                              \
        assert(_var->refcnt != 0);                                      \
        if (--_var->refcnt == 0)                                        \
            _type##_destroy(_var);                                      \
    }                                                                   \
    struct fake
/** @endcond */


/**
 * Generates definitions for allocator functions declared per
 * DECLARE_REFCNT_ALLOCATOR()
 */
#define DEFINE_REFCNT_ALLOCATOR(_scope, _type, _args, _init, _clone, _fini) \
    static inline void _type##_afterclone(_type *_var)                  \
    {                                                                   \
        _var->refcnt = 1;                                               \
        _clone(_var);                                                   \
    }                                                                   \
                                                                        \
    DEFINE_TYPE_ALLOC_COMMON(_scope, _type, _args,                      \
                             do { _OBJHOOK_VARNAME->refcnt = 1; _init; } while(0), \
                             _type##_afterclone,                        \
                             static inline void _type##_destroy,        \
                             _fini);                                    \
    DEFINE_REFCNT_FREE(_scope, _type)

/**
 * Generates definition for the preallocated declared by
 * DECLARE_TYPE_PREALLOC()
 */
#define DEFINE_TYPE_PREALLOC(_scope, _type)                             \
    GENERATED_DEF_##_scope void preallocate_##_type##s(size_t size)     \
    {                                                                   \
        size_t i;                                                       \
        _type *objs = malloc(size * sizeof(*objs));                     \
                                                                        \
        assert(sizeof(*objs) >= sizeof(freelist_t));                    \
        for (i = 0; i < size - 1; i++)                                  \
        {                                                               \
            ((freelist_t *)&objs[i])->chain = (freelist_t *)&objs[i + 1]; \
        }                                                               \
        ((freelist_t *)&objs[size - 1])->chain = freelist_##_type;      \
        freelist_##_type = (freelist_t *)objs;                          \
    }

/** @cond DEV */
/**
 * Generates definitions for allocator functions declared per
 * DECLARE_ARRAY_ALLOCATOR() and family
 */
#define DEFINE_ARRAY_ALLOCATOR(_scope, _name, _type, _scale, _maxsize,  \
                               _init, _clone, _resize, _fini)           \
    ALLOC_COUNTERS(_type, _maxsize + 1);                                \
    static freelist_t *freelists_##_type[_maxsize];                     \
                                                                        \
    ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL            \
    static _type *alloc_##_name(size_t n)                               \
    {                                                                   \
        _type *_var;                                                    \
        size_t _sz = _scale##_order(n);                                 \
                                                                        \
        if (_sz >= _maxsize || freelists_##_type[_sz] == NULL)          \
            _var = frlmalloc(_scale##_size(_sz) * sizeof(_type));       \
        else                                                            \
        {                                                               \
            _var = (_type *)freelists_##_type[_sz];                     \
            freelists_##_type[_sz] = freelists_##_type[_sz]->chain;     \
        }                                                               \
        TRACK_ALLOC_IDX(_type, _sz > _maxsize ? _maxsize : _sz);        \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    GENERATED_DEF_##_scope _type *new_##_name(size_t _n)                \
    {                                                                   \
        _type *_result = alloc_##_type(_n);                             \
        _type *_var;                                                    \
                                                                        \
        for (_var = _result; _var < _result + _n; _var++)               \
        {                                                               \
            _init(_var, _var - _result);                                \
        }                                                               \
        return _result;                                                 \
    }                                                                   \
                                                                        \
    GENERATED_DEF_##_scope _type *unshare_##_name(_type *_var, size_t _n) \
    {                                                                   \
        _type *iter;                                                    \
                                                                        \
        for (iter = _var; iter < _var + _n; _iter++)                    \
        {                                                               \
            _clone(_iter);                                              \
        }                                                               \
        return _var;                                                    \
    }                                                                   \
                                                                        \
    GENERATED_DEF_##_scope _type *copy_##_name(const _type *_var, size_t _n) \
    {                                                                   \
        _type *_result = alloc_##_type(_n);                             \
                                                                        \
        memcpy(_result, _var, _n * sizeof(_type));                      \
        return unshare_##_name(_result, _n);                            \
    }                                                                   \
                                                                        \
    ATTR_NONNULL                                                        \
    static void dispose_##_name(_type *_var, size_t _n)                 \
    {                                                                   \
        size_t _sz = _scale##_order(_n);                                \
                                                                        \
        if(_sz >= _maxsize)                                             \
            free(_var);                                                 \
        else                                                            \
        {                                                               \
            ((freelist_t *)_var)->chain = freelists_##_type[_sz];       \
            freelists_##_type[_sz] = (freelist_t *)_var;                \
        }                                                               \
        TRACK_FREE_IDX(_type, _sz > _maxsize ? _maxsize : _sz);         \
    }                                                                   \
                                                                        \
    GENERATED_DEF_##_scope _type *resize_##_name(_type **_obj, size_t *_oldn, \
                                        size_t _newn)                   \
    {                                                                   \
        size_t old_order;                                               \
        size_t new_order;                                               \
        size_t i;                                                       \
                                                                        \
        if (*_obj == NULL)                                              \
        {                                                               \
            *_oldn = _newn;                                             \
            return (*_obj = new_##_type(_newn));                        \
        }                                                               \
                                                                        \
        old_order = _scale##_order(*_oldn);                             \
        new_order = _scale##_order(_newn);                              \
                                                                        \
        for (i = _newn; _var < *_oldn; i++)                             \
        {                                                               \
            _fini(&(*_obj)[i]);                                          \
        }                                                               \
                                                                        \
        if (new_order < old_order)                                      \
        {                                                               \
            TRACK_FREE_IDX(_type,                                       \
                           old_order > _maxsize ? _maxsize : old_order); \
            TRACK_ALLOC_IDX(_type,                                      \
                            new_order > _maxsize ? _maxsize : new_order); \
        }                                                               \
        else if (new_order > old_order)                                 \
        {                                                               \
            _type *_result = alloc_##_type(_newn);                      \
                                                                        \
            memcpy(_result, *_obj,                                      \
                   *_oldn * sizeof(_type));                             \
            for (i = 0; i < *_oldn; i++)                                \
            {                                                           \
                _resize(&_result[i], &(*_obj)[i]);                      \
            }                                                           \
            dispose_##_type(*_obj);                                     \
            *_obj = _result;                                            \
        }                                                               \
                                                                        \
        for (i = *_oldn; i < _newn; i++)                                \
        {                                                               \
            _init(&_result[i]);                                         \
        }                                                               \
        *_oldn = _newn;                                                 \
        return _result;                                                 \
    }                                                                   \
                                                                        \
    GENERATED_DEF_##_scope free_##_name(_type *_var, size_t n)          \
    {                                                                   \
        size_t i;                                                       \
                                                                        \
        for (i = 0; i < n; i++)                                         \
        {                                                               \
            _fini(&_var[i]);                                            \
        }                                                               \
        dispose_##_type(_var);                                          \
    }                                                                   \
    struct fake
/** @endcond */


#define elt_initialize_to_null(_obj) (void)(*(_obj) = NULL)

#define DEFINE_ARRAY_ALLOCATOR_REFCNT(_scope, _name, _type,             \
                                      _scale, _maxsize)                 \
    static inline void do_clone_elt_##_name(_type **obj)                \
    {                                                                   \
        *obj = copy_##_type(*obj);                                      \
    }                                                                   \
                                                                        \
    static inline void do_free_elt_##_name(_type **obj)                 \
    {                                                                   \
        free_##_type(*obj);                                             \
        obj = NULL;                                                     \
    }                                                                   \
                                                                        \
    DEFINE_ARRAY_ALLOCATOR(_scope, _name, _type *, _scale, _maxsize,    \
                           OBJHOOK0(elt_initialize_to_null),            \
                           do_clone_elt_##_name,                        \
                           trivial_hook2,                               \
                           do_free_elt_##_type)


/**
 * Like `malloc`, but ensures that allocated block will be able to hold
 * freelist_t object
 */
ATTR_MALLOC ATTR_RETURNS_NONNULL
static inline void *frlmalloc(size_t sz)
{
    void *result = malloc(sz > sizeof(freelist_t) ? sz : sizeof(freelist_t));
    assert(result != NULL);
    return result;
}

#define linear_order(_x) (_x)
#define linear_size(_x) (_x)

/**
 * @test
 *  Verify that the logarithmic order is correct
 * `ASSERT_UINT_EQ(log2_order(x), expected);`
 * @testvar{unsigned,x,u}  | @testvar{unsigned,expected}
 * ------------------------|---------------------------
 * `0`                     | `0`
 * `1`                     | `0`
 * `2`                     | `1`
 * `3`                     | `2`
 * `4`                     | `2`
 * `5`                     | `3`
 * `65536`                 | `16`
 */
static inline unsigned log2_order(unsigned x)
{
    return x ? ((unsigned)sizeof(unsigned) * CHAR_BIT -
                count_leading_zeroes(x - 1)) : 0;
}

/**
 * The inverse of log2_order()
 */
#define log2_size(_x) (1u << (_x))

/**
 * Defines a linear allocation scale
 *
 * @test Background:
 * @code
 * DEFINE_LINEAR_SCALE(2);
 * DECLARE_ARRAY_TYPE(simple_linear2_array, void *ptr; unsigned tag;, unsigned);
 * DECLARE_ARRAY_ALLOCATOR(simple_linear2_array);
 *
 * DEFINE_ARRAY_ALLOCATOR(simple_linear2_array, linear2, 4, arr, i,
 *                        {arr->tag = STATE_INITIALIZED;},
 *                        {arr->elts[i] = i; },
 *                        {NEW(arr)->tag = STATE_CLONED; },
 *                        {NEW(arr)->elts[i] = OLD(arr)->elts[i] | STATE_CLONED; },
 *                        {NEW(arr)->tag = STATE_ADJUSTED; },
 *                        {NEW(arr)->elts[i] = OLD(arr)->elts[i] | STATE_ADJUSTED; },
 *                        {arr->tag |= STATE_RESIZED; },
 *                        {arr->tag = STATE_FINALIZED; },
 *                        {arr->elts[i] = STATE_FINALIZED; });
 * @endcode
 *
 * @test
 *  Verify that the linear scaled order is correct
 * `ASSERT_UINT_EQ(linear2_order(x), expected);`
 * @testvar{unsigned,x,u}  | @testvar{unsigned,expected}
 * ------------------------|---------------------------
 * `0`                     | `0`
 * `1`                     | `0`
 * `2`                     | `1`
 * `3`                     | `1`
 * `4`                     | `2`
 * `5`                     | `2`
 * `65536`                 | `32768`
 *
 * @test Alloc and free a scaled-linear array
 * Varying                   | From | To
 * --------------------------|------|-----
 * @testvar{unsigned,size,u} | `0`  |`4`
 * @code
 * simple_linear2_array *prev = NULL;
 * @endcode
 * - Given a fresh array:
 *   `simple_linear2_array *arr = new_simple_linear2_array(size * 2);`
 * - Verify that it is allocated
 *   `ASSERT_PTR_NEQ(arr, NULL);`
 * - Verify that it does not share memory with a lesser array:
 *   `ASSERT_PTR_NEQ(arr, prev);`
 * - Verify that it is initialized:
 *   `ASSERT_UINT_EQ(arr->tag, STATE_INITIALIZED);`
 * - Verify that the number of elements is correct:
 *   `ASSERT_UINT_EQ(arr->nelts, size * 2);`
 * - When it is freed: `free_simple_linear2_array(arr);`
 * - And when it is not so large: `if (size != 4)`
 *    + Verify that it is finalized: `ASSERT_UINT_EQ(arr->tag, STATE_FINALIZED);`
 * - Cleanup: `prev = arr;`
 *
 * @test Alloc, free and realloc a scaled-linear array
 * - Given a fresh array:
 * @code
 * simple_linear2_array *arr = new_simple_linear2_array(size * 2);
 * simple_linear2_array *arr1;
 * @endcode
 * - When it is freed: `free_simple_linear2_array(arr);`
 * - And when a new, slightly larger array, is allocated:
 *   `arr1 = new_simple_linear2_array(size * 2 + 1);`
 * - Then the memory is reused: `ASSERT_PTR_EQ(arr, arr1);`
 * - Cleanup: `free_simple_linear2_array(arr1);`
 * .
 * Varying                   | From | To
 * --------------------------|------|-----
 * @testvar{unsigned,size,u} | `0`  |`3`
 *
 * @test Resize a scaled-linear to a larger size
 * - Given an allocated scaled linear array:
 * @code
 * unsigned sz = ARBITRARY(unsigned, 1, 4);
 * simple_linear2_array *arr = new_simple_linear2_array(sz * 2);
 * @endcode
 * - When it is reallocated to a slightly larger size:
 *   `simple_linear2_array *arr1 = resize_simple_linear2_array(arr, sz * 2 + 1);`
 * - Then the memory is reused:
 *   `ASSERT_PTR_EQ(arr, arr1);`
 * - And the number of elements is correct:
 *   `ASSERT_UINT_EQ(arr1->nelts, sz * 2 + 1);`
 * - And the extra elements are initialized:
 *   `ASSERT_UINT_EQ(arr1->elts[sz * 2], sz * 2);`
 * - When it is reallocate to a yet larger size:
 *   `arr1 = resize_simple_linear2_array(arr, (sz + 1) * 2);`
 * - Then the memory is not reused:
 *   `ASSERT_PTR_NEQ(arr, arr1);`
 * - And the number of elements is correct:
 *   `ASSERT_UINT_EQ(arr1->nelts, (sz + 1) * 2);`
 * - And the extra elements are initialized:
 *   `ASSERT_UINT_EQ(arr1->elts[sz * 2 + 1], sz * 2 + 1);`
 * - Cleanup: `free_simple_linear2_array(arr1);`
 */
#define DEFINE_LINEAR_SCALE(_n)                                 \
    static inline unsigned linear##_n##_order(unsigned x)       \
    {                                                           \
        return x / (_n);                                        \
    }                                                           \
                                                                \
    static inline unsigned linear##_n##_size(unsigned order)    \
    {                                                           \
        return (order + 1) * (_n);                              \
    }                                                           \
    struct fake


#if defined(TEST_ALLOCATOR) || defined(__DOXYGEN__)



DECLARE_TYPE_ALLOCATOR(EXTERN, simple_type, (unsigned tag));
typedef short small_type;


/** @fn new_small_type()
 * @test Allocate and free small
 * - *Given* a new small object:
 * ~~~~~
 * small_type *sm;
 * small_type *sm1;
 * sm = new_small_type();
 * ~~~~~
 * - *Verify* it is initialized:
 * ~~~~~
 * ASSERT_INT_EQ(*sm, (short)STATE_INITIALIZED);
 * ~~~~~
 * - *When* it is freed:
 * ~~~~~
 * *sm = 0; free_small_type(sm);
 * ~~~~~
 * - *And when* a new object is allocated:
 * ~~~~~
 * sm1 = new_small_type();
 * ~~~~~
 * - *Then* the memory is reused:
 * ~~~~~
 * ASSERT_PTR_EQ(sm, sm1);
 * ~~~~~
 * - *And* the second object is initialized:
 * ~~~~~
 * ASSERT_INT_EQ(*sm1, (short)STATE_INITIALIZED);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_small_type(sm1);
 * ~~~~~
 */
DECLARE_TYPE_ALLOCATOR(EXTERN, small_type, ());

typedef struct refcnt_type {
    PROVIDE(REFCNT);
    void *ptr;
    unsigned tag;
} refcnt_type;

/** @fn new_refcnt_type(unsigned)
 * @test Allocate refcounted
 * - *Given* a fresh refcounted object
 * ~~~~~
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * ~~~~~
 * - *Then* it is allocated:
 * ~~~~~
 * ASSERT_PTR_NEQ(rt, NULL);
 * ~~~~~
 * - *And* it is initialized:
 * ~~~~~
 * ASSERT_UINT_EQ(rt->tag, thetag);
 * ~~~~~
 * - *And* its ref counter is `1`:
 * ~~~~~
 * ASSERT_UINT_EQ(rt->refcnt, 1);
 * ~~~~~
 * - *And* the allocated object count is one:
 * ~~~~~
 * ASSERT_UINT_EQ(track_alloc_refcnt_type, 1);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_refcnt_type(rt);
 * ~~~~~
 */
/** @fn free_refcnt_type(refcnt_type *)
 * @test Allocate and free refcounted
 * - *Given* a fresh refcounted object:
 * ~~~~~
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * ~~~~~
 * - *When* it is destroyed:
 * ~~~~~
 * free_refcnt_type(rt);
 * ~~~~~
 * - *Then* it is finalized:
 * ~~~~~
 * ASSERT_UINT_EQ(rt->tag, STATE_FINALIZED);
 * ~~~~~
 * - *And* the allocated object count is zero:
 * ~~~~~
 * ASSERT_UINT_EQ(track_alloc_refcnt_type, 0);
 * ~~~~~
 *
 * @test Allocate, free and reallocate refcounted
 * - *Given* a fresh refcounted object
 * ~~~~~
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * unsigned thetag2 = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * refcnt_type *rt1;
 * ~~~~~
 * - *When* it is freed:
 * ~~~~~
 * free_refcnt_type(rt);
 * ~~~~~
 * - *And when* the new object is allocated:
 * ~~~~~
 * rt1 = new_refcnt_type(thetag2);
 * ~~~~~
 * - *Then* their pointers are the same:
 * ~~~~~
 * ASSERT_PTR_EQ(rt, rt1);
 * ~~~~~
 * - *And* the reference counter is `1`:
 * ~~~~~
 * ASSERT_UINT_EQ(rt1->refcnt, 1);
 * ~~~~~
 * - *And* the object is initialized:
 * ~~~~~
 * ASSERT_UINT_EQ(rt1->tag, thetag2);
 * ~~~~~
 * - *When* the copy is freed:
 * ~~~~~
 * free_refcnt_type(rt1);
 * ~~~~~
 * - *Then* it is finitalized:
 * ~~~~~
 * ASSERT_UINT_EQ(rt1->tag, STATE_FINALIZED);
 * ~~~~~
 */
/** @fn use_refcnt_type(refcnt_type *)
 * @test  Allocate, use and free refcounted
 * - *Given* a fresh refcounted object
 * ~~~~~
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * ~~~~~
 * - *When* it is referenced:
 * ~~~~~
 * refcnt_type *use = use_refcnt_type(rt);
 * ~~~~~
 * - *Then* the referenced pointer is the same:
 * ~~~~~
 * ASSERT_PTR_EQ(use, rt);
 * ~~~~~
 * - *And* the reference counter is incremented by `1`:
 * ~~~~~
 * ASSERT_UINT_EQ(use->refcnt, 2);
 * ~~~~~
 * - *When* it is freed:
 * ~~~~~
 * free_refcnt_type(rt);
 * ~~~~~
 * - *Then* the reference counter is decremented:
 * ~~~~~
 * ASSERT_UINT_EQ(use->refcnt, 1);
 * ~~~~~
 * - *But* the object is not finalized:
 * ~~~~~
 * ASSERT_UINT_EQ(use->tag, thetag);
 * ~~~~~
 * - When the object is freed again:
 * ~~~~~
 * free_refcnt_type(use);
 * ~~~~~
 * - *Then* it is finalized:
 * ~~~~~
 * ASSERT_UINT_EQ(use->tag, STATE_FINALIZED);
 * ~~~~~
 */
/** @fn copy_refcnt_type(const refcnt_type *)
 * @test Copy refcounted
 * - *Given* a fresh refcounted object:
 * ~~~~~
 * unsigned thetag = ARBITRARY(unsigned, 1, 0xfffff);
 * refcnt_type *rt = new_refcnt_type(thetag);
 * ~~~~~
 * - *When* it is copied:
 * ~~~~~
 * refcnt_type *rt1 = copy_refcnt_type(rt);
 * ~~~~~
 * - *Then* the memory is not shared:
 * ~~~~~
 * ASSERT_PTR_NEQ(rt, rt1);
 * ~~~~~
 * - *And* the old object's refcounter is intact:
 * ~~~~~
 * ASSERT_UINT_EQ(rt->refcnt, 1);
 * ~~~~~
 * - *And* the new object's refcounter is `1`:
 * ~~~~~
 * ASSERT_UINT_EQ(rt1->refcnt, 1);
 * ~~~~~
 * - *And* the new object is initialiazed:
 * ~~~~~
 * ASSERT_UINT_EQ(rt1->tag, thetag | STATE_CLONED);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_refcnt_type(rt1);
 * free_refcnt_type(rt);
 * ASSERT_UINT_EQ(track_alloc_refcnt_type, 0);
 * ~~~~~
 */
/** @fn assign_refcnt_type(refcnt_type **, refcnt_type *)
 * @test Assign to NULL
 * - *Given* an empty location:
 * ~~~~~
 * refcnt_type *location = NULL;
 * ~~~~~
 * - *And given* a fresh object:
 * ~~~~~
 * refcnt_type *rt = new_refcnt_type(ARBITRARY(unsigned, 1, 0xffff));
 * ~~~~~
 * - *When* an object is assigned to a location:
 * ~~~~~
 * assign_refcnt_type(&location, rt);
 * ~~~~~
 * - *Then* the location gets equal to the object:
 * ~~~~~
 * ASSERT_PTR_EQ(location, rt);
 * ~~~~~
 * - *And* the reference counter is incremented:
 * ~~~~~
 * ASSERT_UINT_EQ(rt->refcnt, 2);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_refcnt_type(location);
 * free_refcnt_type(rt);
 * ASSERT_UINT_EQ(track_alloc_refcnt_type, 0);
 * ~~~~~
 *
 * @test Assign to itself
 * - *Given* a fresh object:
 * ~~~~~
 * refcnt_type *rt = new_refcnt_type(ARBITRARY(unsigned, 1, 0xffff));
 * refcnt_type *location = rt;
 * ~~~~~
 * - *When* an object is assigned to a location that already references it:
 * ~~~~~
 * assign_refcnt_type(&location, rt);
 * ~~~~~
 * - *Then* the location remains equal to the object:
 * ~~~~~
 * ASSERT_PTR_EQ(location, rt);
 * ~~~~~
 * - *And* the reference counter is unchanged:
 * ~~~~~
 * ASSERT_UINT_EQ(rt->refcnt, 1);
 * ~~~~~
 * - *And* the object is not finalized:
 * ~~~~~
 * ASSERT_UINT_NEQ(rt->tag, STATE_FINALIZED);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_refcnt_type(rt);
 * ASSERT_UINT_EQ(track_alloc_refcnt_type, 0);
 * ~~~~~
 *
 * @test Assign NULL
 * - *Given* a fresh object:
 * ~~~~~
 * refcnt_type *rt = new_refcnt_type(ARBITRARY(unsigned, 1, 0xffff));
 * ~~~~~
 * - *When* `NULL` is assigned to its location:
 * ~~~~~
 * assign_refcnt_type(&rt, NULL);
 * ~~~~~
 * - *Then* the location contains `NULL`:
 * ~~~~~
 * ASSERT_PTR_EQ(location, NULL);
 * ~~~~~
 * - *And* the object is finalized:
 * ~~~~~
 * ASSERT_UINT_EQ(rt->tag, STATE_FINALIZED);
 * ~~~~~
 * - *And* the object is freed:
 * ~~~~~
 * ASSERT_UINT_EQ(track_alloc_refcnt_type, 0);
 * ~~~~~
 */
DECLARE_REFCNT_ALLOCATOR(EXTERN, refcnt_type, (unsigned tag));

/**
 * @test Preallocated items
 * - *Given* two preallocated items:
 * ~~~~~
 * simple_type *st;
 * simple_type *st1;
 * simple_type *st2;
 * preallocate_simple_types(2);
 * ~~~~~
 * - *When* three objects are allocated:
 * ~~~~~
 * st = new_simple_type(ARBITRARY(1, 0xffff));
 * st1 = new_simple_type(ARBITRARY(1, 0xffff));
 * st2 = new_simple_type(ARBITRARY(1, 0xffff));
 * ~~~~~
 * - *Then* the first two are adjacent in memory:
 * ~~~~~
 * ASSERT_PTR_EQ(st1, st + 1);
 * ~~~~~
 * - *But* the last one is not adjacent to either of the previous:
 * ~~~~~
 * ASSERT_PTR_NEQ(st2, st);
 * ASSERT_PTR_NEQ(st2, st1);
 * ASSERT_PTR_NEQ(st2, st1 + 1);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_simple_type2(st);
 * free_simple_type2(st1);
 * free_simple_type2(st2);
 * ASSERT_UINT_EQ(track_alloc_simple_type, 0);
 * ~~~~~
 *
 * @test Preallocate after free
 * - *Given* an object is allocated and freed:
 * ~~~~~
 * simple_type *st;
 * simple_type *st1;
 * simple_type *st2;
 * simple_type *st3;
 * st = new_simple_type(ARBITRARY(1, 0xffff));
 * free_simple_type(st);
 * ~~~~~
 * - *And* given two objects are preallocated:
 * ~~~~~
 * preallocate_simple_types(2);
 * ~~~~~
 * - *When* two objects are allocated:
 * ~~~~~
 * st1 = new_simple_type(ARBITRARY(1, 0xffff));
 * st2 = new_simple_type(ARBITRARY(1, 0xffff));
 * ~~~~~
 * - *Then* they are consecutive:
 * ~~~~~
 * ASSERT_PTR_EQ(st2, st1 + 1);
 * ~~~~~
 * - *And* neither shares memory with the freed object:
 * ~~~~~
 * ASSERT_PTR_NEQ(st1, st);
 * ASSERT_PTR_NEQ(st2, st);
 * ~~~~~
 * - *When* the third object is allocated:
 * ~~~~~
 * st3 = new_simple_type(ARBITRARY(1, 0xffff));
 * ~~~~~
 * - *Then* it shares memory with the freed object:
 * ~~~~~
 * ASSERT_PTR_EQ(st, st3);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_simple_type(st1);
 * free_simple_type(st2);
 * free_simple_type(st3);
 * ASSERT_UINT_EQ(track_alloc_simple_type, 0);
 * ~~~~~
 */
DECLARE_TYPE_PREALLOC(EXTERN, simple_type);

/** @fn new_simple_array(size_t)
 * @test Allocate and free
 * | Varying                  | From|To |
 * |--------------------------|-----|---|
 * | @testvar{unsigned,size,u}|`0`  |`4`|
 * ~~~~~
 * simple_array *prev = NULL;
 * ~~~~~
 * - *Given* an allocated array of a given size:
 * ~~~~~
 * simple_array *arr = new_simple_array(size);
 * unsigned j;
 * ~~~~~
 * - *Then* it is allocated:
 * ~~~~~
 * ASSERT_PTR_NEQ(arr, NULL);
 * ~~~~~
 * - *And* the memory is not shared with an array of lesser size:
 * ~~~~~
 * ASSERT_PTR_NEQ(arr, prev);
 * ~~~~~
 * - *And* the allocated object counter is `1`:
 * ~~~~~
 * ASSERT_UINT_EQ(track_alloc_simple_array[size], 1);
 * ~~~~~
 * - *And* the elements are initialized:
 * ~~~~~
 * for (j = 0; j < size; j++)
 *    ASSERT_UINT_EQ(arr[j], j);
 * ~~~~~
 * - *When* it is destroyed:
 * ~~~~~
 * free_simple_array(arr, size);
 * ~~~~~
 * - *When* the array is allocated through free-lists:
 * ~~~~~
 * if (size != 4)
 * ~~~~~
 *   + *Then* the elements are finalized:
 * ~~~~~
 * for(j = 0; j < size; j++)
 *   ASSERT_UINT_EQ(arr[j], STATE_FINALIZED);
 * ~~~~~
 * - *And* the allocated object count is zero:
 * ~~~~~
 *   ASSERT_UINT_EQ(track_alloc_simple_array[size], 0);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * prev = arr;
 * ~~~~~
 */
/** @fn free_simple_array(simple_type *, size_t)
 * @test Allocate, free and reallocate
 * - *Given* an allocated array of a given size:
 * ~~~~~
 * simple_array *arr = new_simple_array(size);
 * simple_array *arr1;
 * ~~~~~
 * - *When* it is freed:
 * ~~~~~
 * free_simple_array(arr, size);
 * ~~~~~
 * - *And when* a new array of the same size is allocated:
 * ~~~~~
 * arr1 = new_simple_array(size);
 * ~~~~~~
 * - *Then* the memory is shared:
 * ~~~~~~
 * ASSERT_PTR_EQ(arr, arr1);
 * ~~~~~~
 * - Cleanup:
 * ~~~~~~
 * free_simple_array(arr1);
 * ASSERT_UINT_EQ(track_alloc_simple_array[size], 0);
 * ~~~~~~
 * .
 * | Varying                  | From|To |
 * |--------------------------|-----|---|
 * | @testvar{unsigned,size,u}|`0`  |`3`|
 */

/** @fn copy_simple_array(const simple_type *, size_t)
 * @test Copy
 * - *Given* an array:
 * ~~~~~
 * simple_array *arr = new_simple_array(size);
 * unsigned j;
 * ~~~~~
 * - *When* it is copied:
 * ~~~~~
 * simple_array *arr1 = copy_simple_array(arr);
 * ~~~~~
 * - *Then* the memory is not shared:
 * ~~~~~
 * ASSERT_PTR_NEQ(arr, arr1);
 * ~~~~~
 * - And the items are initialized:
 * ~~~~~
 * for(j = 0; j < size; j++)
 *     ASSERT_UINT_EQ(arr1->elts[j], arr->elts[j] | STATE_CLONED);
 * ~~~~~
 * - *When* the copy is freed:
 * ~~~~~
 * free_simple_array(arr1);
 * ~~~~~
 * - Then the original is untouched:
 * ~~~~~
 * for(j = 0; j < size; j++)
 *     ASSERT_UINT_EQ(arr->elts[j], j);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_simple_array(arr);
 * ~~~~~
 * .
 * | Varying                  | From|To |
 * |--------------------------|-----|---|
 * | @testvar{unsigned,size,u}|`0`  |`4`|
 */
/** @fn resize_simple_array(simple_type **, size_t *, size_t)
 *@test Resize Null
 * - *When* a NULL pointer to an array is resized
 * ~~~~~
 * size_t size = 0;
 * simple_array *arr = NULL;
 * unsigned j;
 * simple_array *arr0 = resize_simple_array(&arr, &size, 3);
 * ~~~~~
 * - *Then* the result is not `NULL`:
 * ~~~~~
 * ASSERT_PTR_NEQ(arr, NULL);
 * ~~~~~
 * - *And* the returned and stored pointers are the same:
 * ~~~~~
 * ASSERT_PTR_EQ(arr, arr0);
 * ~~~~~
 * - *And* the stored size is correct:
 * ~~~~~
 * ASSERT_UINT_EQ(size, 3);
 * ~~~~~
 * - *And* the elements are initialized:
 * ~~~~~
 * for (j = 0; j < 3; j++)
 *    ASSERT_UINT_EQ(arr[j], j);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_simple_array(arr);
 * ~~~~~
 *
 * @test Verify that resizing to a lesser size works
 * Varying                  | From | To
 * -------------------------|------|----
 * @testvar{unsigned,size,u}| `3`  | `4`
 * ~~~~~
 * test_resize_smaller_n(size);
 * ~~~~~
 *
 * @test Verify that resizing to a larger size works
 * Varying                  | From | To
 * -------------------------|------|----
 * @testvar{unsigned,size,u}| `2`  | `3`
 * ~~~~~
 * test_resize_larger_n(size);
 * ~~~~~
 *
 * @test Resize and Allocate
 * - *Given* an array
 * ~~~~~
 * unsigned j;
 * size_t size = 2;
 * simple_array *arr = new_simple_array(size);
 * ~~~~~
 * - *When* it is resized:
 * ~~~~~
 * simple_array *arr1 = arr;
 * resize_simple_array(&arr, &size, 3);
 * ~~~~~
 * - *And when* a new array with the same size as the original is allocated:
 * ~~~~~
 * simple_array *arr2 = new_simple_array(2);
 * ~~~~~
 * - *Then* the memory is reused:
 * ~~~~~
 * ASSERT_PTR_EQ(arr2, arr1);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_simple_array(arr);
 * free_simple_array(arr2);
 * for (j = 0; j <= 4; j++)
 *    ASSERT_UINT_EQ(track_alloc_simple_array[j], 0);
 * ~~~~~
 */

/** @fn ensure_simple_array_size(simple_type **, size_t *, size_t, size_t)
 * @test
 * Ensure Size
 * - *Given* an allocated array:
 * ~~~~~
 * size_t sz = ARBITRARY(size_t 1, 4);
 * size_t origsz = sz;
 * size_t delta = ARBITRARY(size_t, 0, 4);
 * simple_array *arr = new_simple_array(sz);
 * simple_array *arr1 = NULL;
 * ~~~~~
 * - *When a lesser size is ensured:
 * ~~~~~
 * arr1 = arr;
 * ensure_simple_array_size(&arr, &sz, sz - 1, 1);
 * ~~~~~
 * - *Then* the array is not reallocated:
 * ~~~~~
 * ASSERT_PTR_EQ(arr, arr1);
 * ~~~~~
 * - *And* the number of elements is unchanged:
 * ~~~~~
 * ASSERT_UINT_EQ(origsz, sz);
 * ~~~~~
 * - *When* a larger size is ensured
 * ~~~~~
 * ensure_simple_array_size(&arr, &sz, sz + 1, delta);
 * - *Then* the array is reallocated:
 * ~~~~~
 * ASSERT_PTR_NEQ(arr, arr1);
 * ~~~~~
 * - *And* the number of elements is incremented:
 * ~~~~~
 * ASSERT_UINT_EQ(sz, origsz + 1 + delta);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_simple_array(arr1);
 * ~~~~~
 */
DECLARE_ARRAY_ALLOCATOR(EXTERN, simple_array, simple_type);

/** @fn new_simple_log_array(size_t)
 * @test Allocate and free (log scale)
 * | Varying                  | From|To |
 * |--------------------------|-----|---|
 * |@testvar{unsigned,order,u}|`0`  |`3`|
 * ~~~~~~
 * simple_log_array *prev = NULL;
 * unsigned j;
 * ~~~~~~
 * - *Given* an allocated array of a certain size:
 * ~~~~~~
 * simple_log_array *arr = new_simple_log_array(1u << order);
 * ~~~~~~
 * - *Verify* it is allocated
 * ~~~~~~
 * ASSERT_PTR_NEQ(arr, NULL);
 * ~~~~~~
 * - *Verify* the memory is not shared with:
 * ~~~~~~
 * ASSERT_PTR_NEQ(arr, prev);
 * ~~~~~~
 * - *Verify* that it is initialized:
 * ~~~~~~
 * for (j = 0; j < (1u << order); j++)
 *   ASSERT_PTR_EQ(arr[j], j);
 * ~~~~~~
 * - *When* it is freed:
 * ~~~~~
 * free_simple_log_array(arr);
 * ~~~~~
 * - *When* it is not so large:
 * ~~~~~
 * if (order != 4)
 * ~~~~~
 *    + *Then* it is finalized:
 * ~~~~~
 * for (j = 0; j < (1u << order); j++)
 *   ASSERT_PTR_EQ(arr[j], STATE_FINALIZED);
 * ~~~~~
 * - *Then* the number of allocated objects is zero:
 * ~~~~~
 * for (j = 0; j <= 4; j++)
 *   ASSERT_UINT_EQ(track_simple_log_array[j], 0);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * prev = arr;
 * ~~~~~
 */
/** @fn free_simple_log_array(simple_type *, size_t)
 * @test Allocate, free and allocate slightly lesser
 * - *Given* an allocated array of a certain size
 * ~~~~~
 * simple_log_array *arr = new_simple_log_array(1u << order);
 * simple_log_array *arr1;
 * ~~~~~
 * - *When* it freed:
 * ~~~~~
 * free_simple_log_array(arr, 1u << order);
 * ~~~~~
 * - *And when* a new array which is slightly less is allocated:
 * ~~~~~
 * arr1 = new_simple_log_array((1u << order) - 1);
 * ~~~~~
 * - *Then* it shares memory with the first array:
 * ~~~~~
 * ASSERT_PTR_EQ(arr, arr1);
 * ~~~~~
 * - Cleanup:
 * ~~~~~
 * free_simple_log_array(arr1, (1u << order) - 1);
 * ~~~~~
 * .
 * | Varying                   | From|To |
 * |---------------------------|-----|---|
 * | @testvar{unsigned,order,u}|`2`  |`3`|
 */
/** @fn resize_simple_log_array(simple_type **, size_t *, size_t)
 * @test Make slightly larger
 * - *Given* an allocated array of a certain size:
 * ~~~~~
 * size_t size = 9;
 * simple_log_array *arr = new_simple_log_array(size);
 * ~~~~~
 * - *When* it is enlarged such that log2 size is still the same:
 * ~~~~~
 * simple_log_array *arr1 = arr;
 * resize_simple_log_array(&arr, &size, 10);
 * ~~~~~
 * - *Then* the memory is not reallocated:
 * ~~~~~
 * ASSERT_PTR_EQ(arr, arr1);
 * ~~~~~
 * - *And* the number of elements is set properly:
 * ~~~~~
 * ASSERT_UINT_EQ(size, 10);
 * ~~~~~
 * - *And* the extra elements are initialized:
 * ~~~~~~
 * ASSERT_UINT_EQ(arr[9], 9);
 * ~~~~~~
 * - Cleanup:
 * ~~~~
 * free_simple_log_array(arr, 10);
 * ~~~~
 */
DECLARE_ARRAY_ALLOCATOR(EXTERN, simple_log_array, simple_type);

#endif

#if defined(TEST_ALLOCATOR)
static inline void simple_type_init(simple_type *obj, unsigned tag)
{
    obj->tag = tag;
}


static inline void simple_type_clone(simple_type *obj)
{
    obj->tag |= STATE_CLONED;
}

static inline void simple_type_fini(simple_type *obj)
{
    obj->tag = STATE_FINALIZED;
}

static inline void simple_type_init_idx(simple_type *obj, size_t idx)
{
    obj->tag = (unsigned)idx;
}

static inline void simple_type_after_resize(simple_type *obj,
                                            simple_type *oldobj ATTR_UNUSED)
{
    obj->tag = STATE_RESIZED;
}

DEFINE_TYPE_ALLOCATOR(EXTERN, simple_type, (unsigned tag), obj,
                      OBJHOOK(simple_type_init, tag),
                      simple_type_clone,
                      simple_type_fini);

static inline void small_type_init(small_type *obj)
{
    *obj = (short)STATE_INITIALIZED;
}

DEFINE_TYPE_ALLOCATOR(EXTERN, small_type, (),
                      OBJHOOK0(small_type_init),
                      trivial_hook, trivial_hook);

static inline void refcnt_type_init(refcnt_type *obj, unsigned tag)
{
    obj->tag = tag;
}


static inline void refcnt_type_clone(refcnt_type *obj)
{
    obj->tag |= STATE_CLONED;
}

static inline void refcnt_type_fini(refcnt_type *obj)
{
    obj->tag = STATE_FINALIZED;
}


DEFINE_REFCNT_ALLOCATOR(EXTERN, refcnt_type, (unsigned tag),
                        OBJHOOK(refcnt_type_init, tag),
                        refcnt_type_clone,
                        refcnt_type_fini);

DEFINE_TYPE_PREALLOC(EXTERN, simple_type);

DEFINE_ARRAY_ALLOCATOR(EXTERN, simple_array, simple_type, linear, 4,
                       simple_type_init_idx,
                       simple_type_clone,
                       simple_type_resize,
                       simple_type_fini);

DEFINE_ARRAY_ALLOCATOR(EXTERN, simple_log_array, simple_type, log2, 4,
                       simple_type_init_idx,
                       simple_type_clone,
                       simple_type_resize,
                       simple_type_fini);

static void test_resize_smaller_n(size_t n)
{
    simple_array *arr = new_simple_array(n);
    simple_array *arr1 = arr;
    size_t newn = n;
    unsigned j;

    resize_simple_array(&arr1, &newn, n - 1);
    ASSERT_PTR_EQ(arr, arr1);
    ASSERT_UINT_EQ(newn, n - 1);
    for (j = 0; j < n - 1; j++)
        ASSERT_UINT_EQ(arr1[j], STATE_RESIZED);
    ASSERT_BITS_EQ(arr1[n - 1], STATE_FINALIZED);

    free_simple_array(arr1, newn);
    for (j = 0; j <= 4; j++)
        ASSERT_UINT_EQ(track_alloc_simple_array[j], 0);
}

static void test_resize_larger_n(size_t n)
{
    simple_array *arr = new_simple_array(n);
    simple_array *arr1 = arr;
    size_t newn = n;
    unsigned j;

    resize_simple_array(arr1, &newn, n + 1);
    ASSERT_PTR_NEQ(arr, arr1);
    ASSERT_UINT_EQ(newn, n + 1);
    for (j = 0; j < n; j++)
        ASSERT_BITS_EQ(arr1[j], STATE_RESIZED);
    ASSERT_UINT_EQ(arr1[n], n);

    free_simple_array(arr1);
    for (j = 0; j <= 4; j++)
        ASSERT_UINT_EQ(track_alloc_simple_array[j], 0);
}


#endif


#endif // defined(ALLOCATOR_IMP)

#endif // 0
