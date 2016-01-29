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
#define ALLOC_POOL_ALIGN_AS double

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
static size_t track_TYPE[MAX_BUCKET + 1];
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
    unsigned tag;
} simple_type;

#define SIMPLE_TYPE_TAG STATIC_ARBITRARY(32)

/* instantiate */
#define ALLOC_TYPE simple_type
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_CODE(_obj)                        \
    do {                                                    \
        (_obj)->state = STATE_INITIALIZED;                  \
        (_obj)->tag = SIMPLE_TYPE_TAG;                      \
    } while(0)
#define ALLOC_COPY_CODE(_obj) (_obj)->state = STATE_COPIED
#define ALLOC_DESTRUCTOR_CODE(_obj) (_obj)->state = STATE_FINALIZED
#define MAX_BUCKET (TESTVAL_SMALL_UINT_MAX)
#include "array_api.h"
#include "array_impl.c"
/* end */

/* END */

/* local */
#define alloc_TYPE ALLOC_PREFIX(alloc)

static returns(not_null) returns(fresh_pointer) returns(important)
ALLOC_TYPE *alloc_TYPE(size_t n)
{
    size_t n_bucket = alloc_calculate_bucket(n, BUCKET_SIZE,
                                             BUCKET_LOG2_SCALE);
    ALLOC_TYPE *obj;

    n = alloc_bucket_size(n_bucket, BUCKET_SIZE,
                          BUCKET_LOG2_SCALE);

#if TRACK_ALLOCATOR
    track_TYPE[n_bucket > MAX_BUCKET ? MAX_BUCKET : n_bucket]++;
#endif

#if USE_ALLOC_POOL
    assert(ALLOC_POOL_PTR != NULL);
    if (n_bucket < MAX_BUCKET)
    {
        PROBE(try_pool_alloc);
        obj = pool_alloc(&ALLOC_POOL_PTR, &ALLOC_POOL_SIZE,
                         sizeof(*obj) * n, sizeof(ALLOC_POOL_ALIGN_AS));
        if (obj != NULL)
        {
            PROBE(pool_alloc);
            return obj;
        }
    }
#endif

    if (n_bucket < MAX_BUCKET && freelist_TYPE[n_bucket] != NULL)
    {
        PROBE(alloc_from_freelist);
        obj = (ALLOC_TYPE *)freelist_TYPE[n_bucket];
        freelist_TYPE[n_bucket] = freelist_TYPE[n_bucket]->chain;

        return obj;
    }

    PROBE(alloc_malloc);
    return frlmalloc(sizeof(*obj) * n);
}

/* local */
#define new_TYPE ALLOC_PREFIX(new)

/* public */
returns(important)
ALLOC_TYPE *new_TYPE(size_t _nelem)
{
    if (_nelem == 0)
        return NULL;
    else
    {
        ALLOC_TYPE *_obj = alloc_TYPE(_nelem);
        size_t _i;

        PROBE(initialize_object);
        for (_i = 0; _i < _nelem; _i++)
        {
            ALLOC_CONSTRUCTOR_CODE((&_obj[_i]));
        }
        return _obj;
    }
}

/*! Test: Allocate zero-sized array
 */
static void allocate_zero(void)
{
    ASSERT_NULL(new_simple_type(0));
}

/* local */
#define dispose_TYPE ALLOC_PREFIX(dispose)

static arguments(not_null)
void dispose_TYPE(size_t nelem, ALLOC_TYPE obj[var_size(nelem)])
{
    size_t n_bucket = alloc_calculate_bucket(nelem, BUCKET_SIZE,
                                             BUCKET_LOG2_SCALE);

#if TRACK_ALLOCATOR
    assert(track_TYPE[n_bucket > MAX_BUCKET ? MAX_BUCKET : n_bucket] > 0);
    track_TYPE[n_bucket > MAX_BUCKET ? MAX_BUCKET : n_bucket]--;
#endif

    if (n_bucket >= MAX_BUCKET)
    {
        PROBE(free_large);
        free(obj);
        return;
    }

#if USE_ALLOC_POOL
    nelem = alloc_bucket_size(n_bucket, BUCKET_SIZE, BUCKET_LOG2_SCALE);
    if (pool_maybe_free(&ALLOC_POOL_PTR, &ALLOC_POOL_SIZE,
                        obj, sizeof(*obj) * nelem,
                        sizeof(ALLOC_POOL_ALIGN_AS)))
    {
        PROBE(free_to_pool);
        return;
    }
    PROBE(fail_free_to_pool);
#endif
    PROBE(free_to_list);
    ((freelist_t *)obj)->chain = freelist_TYPE[n_bucket];
    freelist_TYPE[n_bucket] = (freelist_t *)obj;
}


/* local */
#define free_TYPE ALLOC_PREFIX(free)

/* public */
void free_TYPE(size_t _nelem, ALLOC_TYPE _obj[var_size(_nelem)])
{
    size_t _i;

    if (_obj == NULL) {
        PROBE(free_null);
        return;
    }

    for (_i = 0; _i < _nelem; _i++)
    {
        ALLOC_DESTRUCTOR_CODE((&_obj[_i]));
    }

    dispose_TYPE(_nelem, _obj);
}

/*! Test: Free NULL
 */
static void free_null(void)
{
    freelist_t *prev = freelist_simple_type[0];
    free_simple_type(0, NULL);
    ASSERT_EQ(ptr, freelist_simple_type[0], prev);
    ASSERT_EQ(unsigned, track_simple_type[0], 0);
}


/*! Test: Allocate and free an object
 */
static void allocate_and_free(testval_small_uint_t n)
{
    simple_type *st = new_simple_type(n + 1);
    unsigned i;

    ASSERT_NOT_NULL(st);
    ASSERT_EQ(unsigned, track_simple_type[n], 1);
    for (i = 0; i < n + 1; i++)
    {
        ASSERT_EQ(unsigned, st[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, st[i].state, STATE_INITIALIZED);
    }

    free_simple_type(n + 1, st);
    if (n != TESTVAL_SMALL_UINT_MAX)
    {
        ASSERT_EQ(ptr, freelist_simple_type[n], (void *)st);
        for (i = (sizeof(void *) + sizeof(simple_type) - 1) /
                 sizeof(simple_type);
             i < n + 1;
             i++)
        {
            ASSERT_EQ(unsigned, st[i].state, STATE_FINALIZED);
        }
    }
    ASSERT_EQ(unsigned, track_simple_type[n], 0);
}


/*! Test: Allocate and free an object and allocate another
 */
static void allocate_free_allocate(testval_small_uint_t n)
{
    if (n == 0)
        return;
    else
    {
        simple_type *st = new_simple_type(n);
        simple_type *st1;
        unsigned i;

        free_simple_type(n, st);
        st1 = new_simple_type(n);
        ASSERT_EQ(ptr, st, st1);
        ASSERT_NULL(freelist_simple_type[n - 1]);
        for (i = 0; i < n; i++)
            ASSERT_EQ(unsigned, st1[i].state, STATE_INITIALIZED);
        ASSERT_EQ(unsigned, track_simple_type[n - 1], 1);
        free_simple_type(n, st1);
    }
}

/* local */
#define copy_TYPE ALLOC_PREFIX(copy)

/* public */
returns(important)
ALLOC_TYPE *copy_TYPE(size_t _nelem,
                      const ALLOC_TYPE _orig[var_size(_nelem)])
{
    if (_nelem == 0)
        return NULL;
    else
    {
        ALLOC_TYPE *_copy = alloc_TYPE(_nelem);
        size_t _i;

        assert(_orig != NULL);
        PROBE(do_copy);
        memcpy(_copy, _orig, sizeof(*_copy) * _nelem);
        for (_i = 0; _i < _nelem; _i++)
        {
            ALLOC_COPY_CODE((&_copy[_i]));
        }
        return _copy;
    }
}

/*! Test: Copy
 */
static void test_copy(testval_small_uint_t n)
{
    simple_type *st = new_simple_type(n + 1);
    simple_type *st1;
    unsigned i;

    st1 = copy_simple_type(n + 1, st);
    ASSERT_NEQ(ptr, st, st1);

    for (i = 0; i < n + 1; i++)
    {
        ASSERT_EQ(unsigned, st1[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, st1[i].state, STATE_COPIED);
    }
    ASSERT_EQ(unsigned, track_simple_type[n], 2);
    free_simple_type(n + 1, st);
    free_simple_type(n + 1, st1);
    ASSERT_EQ(unsigned, track_simple_type[n], 0);
}

/*! Test: Deallocate and copy
 */
static void deallocate_and_copy(testval_small_uint_t n)
{
    if (n == TESTVAL_SMALL_UINT_MAX)
        return;
    else
    {
        simple_type *st = new_simple_type(n + 1);
        simple_type *st1 = new_simple_type(n + 1);
        simple_type *st2 = NULL;

        free_simple_type(n + 1, st);
        st2 = copy_simple_type(n + 1, st1);
        ASSERT_EQ(ptr, st2, st);
        free_simple_type(n + 1, st1);
        free_simple_type(n + 1, st2);
        ASSERT_EQ(unsigned, track_simple_type[n], 0);
    }
}


/* TESTS */

static void *shared_pool_ptr;
static size_t shared_pool_size;

/* instantiate */
#define ALLOC_TYPE short
#define ALLOC_NAME pool_short
#define USE_ALLOC_POOL true
#define ALLOC_POOL_PTR shared_pool_ptr
#define ALLOC_POOL_SIZE shared_pool_size
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_CODE(_obj) (*(_obj) = (short)STATE_INITIALIZED)
#define MAX_BUCKET (TESTVAL_SMALL_UINT_MAX)
#include "array_api.h"
#include "array_impl.c"
/* end */

/*! Test: Allocate from pool
 */
static void alloc_from_pool(testval_small_uint_t n)
{
    short *st;
    static void *pool_base;
    void *current_pool;
    unsigned i;

    if (n == 0)
    {
        /* that's the space for arrays up to 10 short's +
         * double-boundary alignment
         */
        shared_pool_size = 4 * sizeof(double) + 4 * 2 * sizeof(double) +
            3 * sizeof(double);
        pool_base = shared_pool_ptr = malloc(shared_pool_size);
    }

    current_pool = shared_pool_ptr;
    st = new_pool_short(n + 1);
    ASSERT_NOT_NULL(st);
    for (i = 0; i < n + 1; i++)
        ASSERT_EQ(int, st[i], (short)STATE_INITIALIZED);
    if (n < TESTVAL_SMALL_UINT_MAX)
        ASSERT_EQ(ptr, st, current_pool);
    else
    {
        ASSERT_NEQ(ptr, st, current_pool);
        free_pool_short(n + 1, st);
    }

    ASSERT_EQ(unsigned, (uintptr_t)st % sizeof(double), 0);

    if (n == TESTVAL_SMALL_UINT_MAX)
    {
        ASSERT_EQ(unsigned, shared_pool_size, 0);
        free(pool_base);
        pool_base = NULL;
    }
}

/*! Test: Allocate from pool and immediately free
 */
static void alloc_from_pool_and_return(testval_small_uint_t n)
{
    short *st;
    static void *pool_base;


    if (n == 0)
    {
        shared_pool_size = 3 * sizeof(double);
        pool_base = shared_pool_ptr = malloc(shared_pool_size);
    }
    st = new_pool_short(n + 1);
    if (n < TESTVAL_SMALL_UINT_MAX)
        ASSERT_EQ(ptr, st, pool_base);
    free_pool_short(n + 1, st);
    ASSERT_EQ(unsigned, shared_pool_size, 3 * sizeof(double));
    ASSERT_NULL(freelist_pool_short[n]);

    if (n == TESTVAL_SMALL_UINT_MAX)
    {
        free(pool_base);
        pool_base = NULL;
    }
}

/*! Test: Malloc when no space in the pool
 */
static void alloc_not_from_pool(testval_small_uint_t n)
{
    void *pool_base;
    short *st;

    shared_pool_size = sizeof(short) * (n + 1) - 1;
    shared_pool_ptr = pool_base = malloc(shared_pool_size);
    st = new_pool_short(n + 1);
    ASSERT_NOT_NULL(st);
    ASSERT_NEQ(ptr, st, pool_base);
    ASSERT_EQ(ptr, pool_base, shared_pool_ptr);
    ASSERT_EQ(unsigned, shared_pool_size, sizeof(short) * (n + 1) - 1);
    free_pool_short(n + 1, st);
    ASSERT_EQ(ptr, pool_base, shared_pool_ptr);
    ASSERT_EQ(unsigned, shared_pool_size, sizeof(short) * (n + 1) - 1);
    if (n < TESTVAL_SMALL_UINT_MAX)
        ASSERT_EQ(ptr, freelist_pool_short[n], st);
    free(pool_base);
}

/* END */

/* local */
#define grow_TYPE ALLOC_PREFIX(grow)

/* public */
arguments(not_null)
ALLOC_TYPE *grow_TYPE(size_t * restrict nelem,
                      ALLOC_TYPE * restrict * restrict items,
                      size_t incr)
{
    assert(items != NULL);
    size_t old_bucket;
    size_t new_bucket;
    size_t i;

    if (incr == 0)
    {
        PROBE(no_grow);
        return *items;
    }
    if (*items == NULL)
    {
        PROBE(grow_from_null);
        *items = new_TYPE(incr);
        *nelem = incr;
        return *items;
    }
    old_bucket = alloc_calculate_bucket(*nelem, BUCKET_SIZE,
                                        BUCKET_LOG2_SCALE);
    new_bucket = alloc_calculate_bucket(*nelem + incr, BUCKET_SIZE,
                                        BUCKET_LOG2_SCALE);
    assert(new_bucket >= old_bucket);
    if (old_bucket >= MAX_BUCKET || old_bucket != new_bucket)
    {
        ALLOC_TYPE *new_items = alloc_TYPE(*nelem + incr);

        memcpy(new_items, *items, sizeof(**items) * *nelem);
        dispose_TYPE(*nelem, *items);
        *items = new_items;
        PROBE(grow_new_bucket);
    }
    for (i = 0; i < *nelem + incr; i++)
    {
        ALLOC_CONSTRUCTOR_CODE((&(*items)[i]));
    }
    PROBE(do_grow);
    *nelem += incr;
    return *items + *nelem - incr;
}

/*! Test: Grow with zero increment
 */
static void grow_by_zero(testval_small_uint_t n)
{
    size_t sz = n;
    simple_type *st = new_simple_type(n);
    simple_type *gst = st;
    simple_type *gst2 = grow_simple_type(&sz, &gst, 0);

    ASSERT_EQ(ptr, gst, st);
    ASSERT_EQ(ptr, gst, gst2);

    free_simple_type(n, gst);
}

/*! Test: Grow by one
 */
static void grow_by_one(testval_small_uint_t n)
{
    size_t sz = n;
    simple_type *st = new_simple_type(n);
    simple_type *gst = st;
    simple_type *gst2 = grow_simple_type(&sz, &gst, 1);
    size_t i;

    ASSERT_NOT_NULL(gst);
    ASSERT_NEQ(ptr, gst, st);
    ASSERT_EQ(unsigned, sz, n + 1);
    ASSERT_EQ(ptr, gst2, gst + n);

    if (n != 0)
        ASSERT_EQ(ptr, freelist_simple_type[n - 1], st);
    for (i = 0; i < sz; i++)
    {
        ASSERT_EQ(unsigned, gst[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, gst[i].state, STATE_INITIALIZED);
    }
    free_simple_type(sz, gst);
    if (n != TESTVAL_SMALL_UINT_MAX)
        ASSERT_EQ(ptr, freelist_simple_type[n], gst);
}

/* local */
#define ensure_TYPE_size QNAME(ALLOC_PREFIX(ensure), size)

/* public */
static inline arguments(not_null)
ALLOC_TYPE *ensure_TYPE_size(size_t * restrict nelem,
                             ALLOC_TYPE * restrict * restrict items,
                             size_t req_size,
                             size_t reserve)
{
    if (*nelem >= req_size)
    {
        PROBE(no_need_to_grow);
        return *items;
    }
    PROBE(need_to_grow);
    grow_TYPE(nelem, items, req_size - *nelem + reserve);
    return *items;
}

/*! Test: Ensure size
 */
static void test_ensure_size(testval_small_uint_t n)
{
    size_t sz = n;
    simple_type *st = new_simple_type(n);
    simple_type *gst = st;
    simple_type *gst2 = ensure_simple_type_size(&sz, &gst, n + 1, 1);

    ASSERT_EQ(unsigned, sz, n + 2);
    ASSERT_EQ(ptr, gst, gst2);
    ASSERT_NEQ(ptr, st, gst);

    st = gst;
    gst2 = ensure_simple_type_size(&sz, &gst, n + 2, 1);
    ASSERT_EQ(unsigned, sz, n + 2);
    ASSERT_EQ(ptr, gst2, gst);
    ASSERT_EQ(ptr, gst, st);

    gst2 = ensure_simple_type_size(&sz, &gst, n, 1);
    ASSERT_EQ(unsigned, sz, n + 2);
    ASSERT_EQ(ptr, gst2, gst);
    ASSERT_EQ(ptr, gst, st);

    free_simple_type(sz, gst);
}

/* TESTS */
#define SIMPLE_TYPE_BUCKET_SIZE 1024

/* instantiate */
#define ALLOC_TYPE simple_type
#define ALLOC_NAME simple_type_page
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_CODE(_obj)                        \
    do {                                                    \
        (_obj)->state = STATE_INITIALIZED;                  \
        (_obj)->tag = SIMPLE_TYPE_TAG;                      \
    } while(0)
#define ALLOC_COPY_CODE(_obj) (_obj)->state = STATE_COPIED
#define ALLOC_DESTRUCTOR_CODE(_obj) (_obj)->state = STATE_FINALIZED
#define BUCKET_SIZE SIMPLE_TYPE_BUCKET_SIZE
#define MAX_BUCKET (TESTVAL_SMALL_UINT_MAX + 1)
#include "array_api.h"
#include "array_impl.c"
/* end */

/*! Test: Grow by one
 */
static void alloc_and_grow_by_page(testval_small_uint_t n)
{
    size_t initial_sz = (n + 1) * SIMPLE_TYPE_BUCKET_SIZE;
    size_t sz = initial_sz;
    simple_type *st = new_simple_type_page(sz);
    simple_type *gst = st;
    simple_type *gst2;
    size_t i;

    ASSERT_NOT_NULL(st);

    gst2 = grow_simple_type_page(&sz, &gst, 1);
    ASSERT_EQ(ptr, gst, st);
    ASSERT_EQ(unsigned, sz, initial_sz + 1);
    ASSERT_EQ(ptr, gst2, gst + initial_sz);

    gst2 = grow_simple_type_page(&sz, &gst, SIMPLE_TYPE_BUCKET_SIZE - 2);
    ASSERT_EQ(ptr, gst, st);
    ASSERT_EQ(unsigned, sz, initial_sz + SIMPLE_TYPE_BUCKET_SIZE - 1);
    ASSERT_EQ(ptr, gst2, gst + initial_sz + 1);

    gst2 = grow_simple_type_page(&sz, &gst, 1);
    ASSERT_NEQ(ptr, gst, st);
    ASSERT_EQ(unsigned, sz, initial_sz + SIMPLE_TYPE_BUCKET_SIZE);
    ASSERT_EQ(ptr, gst2, gst + initial_sz + SIMPLE_TYPE_BUCKET_SIZE - 1);
    ASSERT_EQ(ptr, freelist_simple_type[n], st);

    for (i = 0; i < sz; i++)
    {
        ASSERT_EQ(unsigned, gst[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, gst[i].state, STATE_INITIALIZED);
    }
    free_simple_type_page(sz, gst);
    if (n != TESTVAL_SMALL_UINT_MAX)
        ASSERT_EQ(ptr, freelist_simple_type[n + 1], gst);
}



#if 0

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
