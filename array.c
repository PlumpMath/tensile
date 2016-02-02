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

/* parameter */
#define IS_MONOID false

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
#define IS_MONOID true
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
#define unshare_TYPE ALLOC_PREFIX(unshare)

/* public */
static inline arguments(not_null)
void unshare_TYPE(size_t _nelem, unused ALLOC_TYPE *_obj)
{
    size_t _i;

    PROBE(unshare_object);
    for (_i = 0; _i < _nelem; _i++)
    {
        ALLOC_COPY_CODE((&_obj[_i]));
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

        assert(_orig != NULL);
        PROBE(do_copy);
        memcpy(_copy, _orig, sizeof(*_copy) * _nelem);
        unshare_TYPE(_nelem, _copy);
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

/*! Test: Grow by page
 */
static void alloc_and_grow_by_page(testval_small_uint_t n)
{
    size_t initial_sz = (n + 1) * SIMPLE_TYPE_BUCKET_SIZE - 1;
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

    gst2 = grow_simple_type_page(&sz, &gst, 1);
    ASSERT_NEQ(ptr, gst, st);
    ASSERT_EQ(unsigned, sz, initial_sz + 2);
    ASSERT_EQ(ptr, gst2, gst + initial_sz + 1);
    ASSERT_EQ(ptr, freelist_simple_type_page[n], st);
    ASSERT_EQ(unsigned, track_simple_type_page[n], 0);
    ASSERT_EQ(unsigned, track_simple_type_page[n + 1], 1);

    if (n != TESTVAL_SMALL_UINT_MAX)
    {
        st = gst;
        gst2 = grow_simple_type_page(&sz, &gst, SIMPLE_TYPE_BUCKET_SIZE - 1);
        ASSERT_EQ(ptr, gst, st);
        ASSERT_EQ(unsigned, sz, initial_sz + SIMPLE_TYPE_BUCKET_SIZE + 1);
        ASSERT_EQ(ptr, gst2, gst + initial_sz + 2);
    }

    for (i = 0; i < sz; i++)
    {
        ASSERT_EQ(unsigned, gst[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, gst[i].state, STATE_INITIALIZED);
    }
    free_simple_type_page(sz, gst);
    if (n != TESTVAL_SMALL_UINT_MAX)
        ASSERT_EQ(ptr, freelist_simple_type_page[n + 1], gst);
}

/*! Test: Grow by page large
 */
static void alloc_and_grow_by_page_large(void)
{
    size_t sz = (TESTVAL_SMALL_UINT_MAX + 1) * SIMPLE_TYPE_BUCKET_SIZE + 1;
    simple_type *st = new_simple_type_page(sz);
    simple_type *gst = st;
    size_t *tracker = &track_simple_type_page[TESTVAL_SMALL_UINT_MAX + 1];

    ASSERT_EQ(unsigned, *tracker, 1);
    grow_simple_type_page(&sz, &gst, 1);
    ASSERT_NEQ(ptr, gst, st);
    ASSERT_EQ(unsigned, *tracker, 1);
    st = gst;
    grow_simple_type_page(&sz, &gst, SIMPLE_TYPE_BUCKET_SIZE);
    ASSERT_NEQ(ptr, gst, st);
    ASSERT_EQ(unsigned, *tracker, 1);

    free_simple_type_page(sz, gst);
    ASSERT_EQ(unsigned, *tracker, 0);
}

/* instantiate */
#define ALLOC_TYPE simple_type
#define ALLOC_NAME simple_type_pageorder
#define TRACK_ALLOCATOR true
#define ALLOC_CONSTRUCTOR_CODE(_obj)                        \
    do {                                                    \
        (_obj)->state = STATE_INITIALIZED;                  \
        (_obj)->tag = SIMPLE_TYPE_TAG;                      \
    } while(0)
#define ALLOC_COPY_CODE(_obj) (_obj)->state = STATE_COPIED
#define ALLOC_DESTRUCTOR_CODE(_obj) (_obj)->state = STATE_FINALIZED
#define BUCKET_SIZE SIMPLE_TYPE_BUCKET_SIZE
#define BUCKET_LOG2_SCALE true
#define MAX_BUCKET (TESTVAL_SMALL_UINT_MAX + 1)
#include "array_api.h"
#include "array_impl.c"
/* end */


/*! Test: Grow by page logarithmically
 */
static void alloc_and_grow_by_pageorder(testval_small_uint_t n)
{
    size_t initial_sz = (1ul << n) * SIMPLE_TYPE_BUCKET_SIZE;
    size_t sz = initial_sz;
    simple_type *st = new_simple_type_pageorder(sz);
    simple_type *gst = st;
    simple_type *gst2;
    size_t i;

    ASSERT_NOT_NULL(st);

    gst2 = grow_simple_type_pageorder(&sz, &gst, 1);
    ASSERT_NEQ(ptr, gst, st);
    ASSERT_EQ(unsigned, sz, initial_sz + 1);
    ASSERT_EQ(ptr, gst2, gst + initial_sz);
    ASSERT_EQ(ptr, freelist_simple_type_pageorder[n], st);
    ASSERT_EQ(unsigned, track_simple_type_pageorder[n], 0);
    ASSERT_EQ(unsigned, track_simple_type_pageorder[n + 1], 1);

    if (n != 0)
    {
        const size_t incr = (1ul << (n - 1)) * SIMPLE_TYPE_BUCKET_SIZE;


        st = gst;
        gst2 = grow_simple_type_pageorder(&sz, &gst, incr);
        if (n == TESTVAL_SMALL_UINT_MAX)
            ASSERT_NEQ(ptr, gst, st);
        else
            ASSERT_EQ(ptr, gst, st); 
        ASSERT_EQ(unsigned, sz, initial_sz + 1 + incr);
        ASSERT_EQ(ptr, gst2, gst + initial_sz + 1);
    }
    
    for (i = 0; i < sz; i++)
    {
        ASSERT_EQ(unsigned, gst[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, gst[i].state, STATE_INITIALIZED);
    }

    free_simple_type_pageorder(sz, gst);
    if (n != TESTVAL_SMALL_UINT_MAX)
        ASSERT_EQ(ptr, freelist_simple_type_pageorder[n + 1], gst);
}

/* END */

/* public */
#if IS_MONOID

/* local */
#define append_TYPE ALLOC_PREFIX(append)

/* public */
argument(not_null,1,2)
void append_TYPE(size_t * restrict dest_sz, ALLOC_TYPE ** restrict dest,
                 size_t app_sz, const ALLOC_TYPE *app)
{
    ALLOC_TYPE *tail;
    
    if (app_sz == 0)
    {
        PROBE(append_zero);
        return;
    }
    
    assert(app != NULL);
    if (app == *dest)
    {
        PROBE(append_duplicate);
        assert(app_sz <= *dest_sz);
        tail = grow_TYPE(dest_sz, dest, app_sz);
        memcpy(tail, *dest, app_sz * sizeof(ALLOC_TYPE));
    }
    else
    {
        PROBE(append_normal);
        tail = grow_TYPE(dest_sz, dest, app_sz);
        memcpy(tail, app, app_sz * sizeof(ALLOC_TYPE));
    }
    unshare_TYPE(app_sz, tail);
}

/*! Test: Do append zero
 */
static void do_append_zero(testval_small_uint_t n)
{
    size_t sz = n;
    simple_type *st = new_simple_type(sz);
    simple_type *dest = st;
    size_t i;

    append_simple_type(&sz, &dest, 0, NULL);
    ASSERT_EQ(ptr, st, dest);
    ASSERT_EQ(unsigned, sz, n);
    for (i = 0; i < n; i++)
    {
        ASSERT_EQ(unsigned, st[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, st[i].state, STATE_INITIALIZED);
    }
    free_simple_type(sz, st);
}

/*! Test: Do append itself
 */
static void do_append_itself(testval_small_uint_t n)
{
    size_t sz = n + 1;
    simple_type *dest = new_simple_type(sz);
    size_t i;

    append_simple_type(&sz, &dest, sz, dest);
    ASSERT_EQ(unsigned, sz, (n + 1) * 2);
    for (i = 0; i < sz; i++)
    {
        ASSERT_EQ(unsigned, dest[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, dest[i].state,
                  i < n + 1 ? STATE_INITIALIZED : STATE_COPIED);
    }
    free_simple_type(sz, dest);
}

/*! Test: Do append
 */
static void do_append(testval_small_uint_t n, testval_small_uint_t m)
{
    size_t sz = n;
    simple_type *dest = new_simple_type(sz);
    simple_type *app = new_simple_type(m + 1);
    size_t i;

    for (i = 0; i < m + 1; i++)
        app[i].tag = ~app[i].tag;
    append_simple_type(&sz, &dest, m + 1, app);
    ASSERT_NOT_NULL(dest);
    ASSERT_EQ(unsigned, sz, n + m + 1);
    for (i = 0; i < n; i++)
    {
        ASSERT_EQ(unsigned, dest[i].tag, SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, dest[i].state, STATE_INITIALIZED);
    }
    for (i = 0; i < m + 1; i++)
    {
        ASSERT_EQ(unsigned, dest[n + i].tag, ~SIMPLE_TYPE_TAG);
        ASSERT_EQ(unsigned, dest[n + i].state, STATE_COPIED);
    }
    free_simple_type(sz, dest);
    free_simple_type(m + 1, app);
}


/* local */
#define concat_TYPE ALLOC_PREFIX(concat)

/* public */
argument(not_null,5) returns(important)
ALLOC_TYPE *concat_TYPE(size_t first_sz, const ALLOC_TYPE *first,
                        size_t second_sz, const ALLOC_TYPE *second,
                        size_t * restrict dest_sz)
{
    ALLOC_TYPE *conc;
    
    if (first_sz == 0)
    {
        PROBE(conc_first_null);
        
        *dest_sz = second_sz;
        return copy_TYPE(second_sz, second);
    }
    if (second_sz == 0)
    {
        PROBE(conc_second_null);
        
        *dest_sz = first_sz;
        return copy_TYPE(first_sz, first);
    }

    assert(first != NULL);
    assert(second != NULL);

    PROBE(conc_two);
    
    conc = alloc_TYPE(first_sz + second_sz);
    memcpy(conc, first, first_sz * sizeof(ALLOC_TYPE));
    memcpy(conc + first_sz, second, second_sz * sizeof(ALLOC_TYPE));
    unshare_TYPE(first_sz + second_sz, conc);
    *dest_sz = first_sz + second_sz;
    return conc;
}

/*! Test: Test concatenate
 */
static void do_concatenate(testval_small_uint_t n, testval_small_uint_t m)
{
    simple_type *st1 = new_simple_type(n);
    simple_type *st2 = new_simple_type(m);
    size_t conc_sz = 0;
    simple_type *conc;
    size_t i;

    for (i = 0; i < m; i++)
        st2[i].tag = ~st2[i].tag;
    
    conc = concat_simple_type(n, st1, m, st2, &conc_sz);
    
    if (n == 0 && m == 0)
        ASSERT_NULL(conc);
    else
    {
        ASSERT_NEQ(ptr, st1, conc);
        ASSERT_NEQ(ptr, st2, conc);
    }
    ASSERT_EQ(unsigned, conc_sz, n + m);
    for (i = 0; i < conc_sz; i++)
    {
        ASSERT_EQ(unsigned, conc[i].state, STATE_COPIED);
        ASSERT_EQ(unsigned, conc[i].tag,
                  i < n ? SIMPLE_TYPE_TAG : ~SIMPLE_TYPE_TAG);
    }
    free_simple_type(n, st1);
    free_simple_type(m, st2);
    free_simple_type(conc_sz, conc);
    ASSERT_EQ(unsigned, track_simple_type[conc_sz > TESTVAL_SMALL_UINT_MAX ?
                                          TESTVAL_SMALL_UINT_MAX :
                                          conc_sz], 0);
}

#endif
