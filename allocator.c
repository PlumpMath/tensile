/*
 * Copyright  * (c) 2016  Artem V. Andreev
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
/** @file
 * @brief Type-independent internal allocator code
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#include "compiler.h"
#include "bitops.h"
#include "allocator.h"
#include "assertions.h"

annotation(returns, fresh_pointer) annotation(returns, not_null)
annotation(returns, important)
static void *frlmalloc(size_t sz)
{
    void *result = malloc(sz > sizeof(freelist_t) ? sz : sizeof(freelist_t));
    if (result == NULL)
        abort();
    
    return result;
}

annotation(returns, important) annotation(arguments, not_null)
annotation(argument, storage_size, 2)
annotation(argument, storage_align, 3)
static void *pool_alloc(shared_pool_t *pool, size_t objsize, size_t align)
{
    size_t misalign = (align - (uintptr_t)pool->current % align) % align;
    size_t alloc_size = objsize + misalign;
    void *result;

    if (pool->available < alloc_size)
        return NULL;
    result = (uint8_t *)pool->current + misalign;
    pool->current = (uint8_t *)pool->current + alloc_size;
    pool->available -= alloc_size;

    return result;
}

annotation(arguments, not_null)
annotation(returns, important)
static bool pool_maybe_free(shared_pool_t *pool, void *obj,
                            size_t objsize, size_t align)
{
    if (pool->current == (uint8_t *)obj + objsize)
    {
        size_t alloc_size = (objsize + align - 1) / align * align;
        
        pool->current = (uint8_t *)pool->current - alloc_size;
        pool->available += alloc_size;
        return true;
    }
    return false;
}

annotation(global_state, none) annotation(returns, important)
static size_t alloc_calculate_bucket(allocator_t *alloc, size_t n)
{
    assert(n > 0);
    n = (n - 1) / alloc->bucket_size;
    if (alloc->log2_bucket)
        n = sizeof(n) * CHAR_BIT - count_leading_zeroes(n);
    return n;
}

annotation(global_state, none) annotation(returns, important)
static size_t alloc_bucket_size(allocator_t *alloc, size_t n_bucket)
{
    if (alloc->log2_bucket)
        n_bucket = 1u << n_bucket;
    else
        n_bucket++;
    return n_bucket * alloc->bucket_size;
}

void *alloc_storage(allocator_t *alloc, size_t n)
{
    size_t n_bucket;
    void *obj;

    if (n == 0)
        return NULL;
    
    n_bucket = alloc_calculate_bucket(alloc, n);

    if (n_bucket >= alloc->n_buckets)
        alloc->n_large_alloc++;
    else
    {
        n = alloc_bucket_size(alloc, n_bucket);
        alloc->buckets[n_bucket].n_alloc++;

        if (alloc->pool)
        {
            obj = pool_alloc(alloc->pool, alloc->elt_size * n,
                             alloc->elt_size > sizeof(double) ?
                             sizeof(double) : alloc->elt_size);
            if (obj != NULL)
                return obj;
        }
        obj = alloc->buckets[n_bucket].freelist;
        if (obj != NULL)
        {
            alloc->buckets[n_bucket].freelist =
                alloc->buckets[n_bucket].freelist->chain;
            return obj;
        }
    }

    return frlmalloc(alloc->elt_size * n);
}

#if DO_TESTS

#define ALLOC_SIZE 16
#define ALLOC_N_BUCKETS (TESTVAL_SMALL_UINT_MAX + 1)

static allocator_t test_allocator =
    INIT_ALLOCATOR(ALLOC_SIZE, ALLOC_N_BUCKETS, 1, false, NULL);

/** @testcase Allocate zero-sized array */
TEST_SPEC(allocate_zero, "Allocate zero-sized array", false,
          {
              ASSERT_NULL(alloc_storage(&test_allocator, 0));
          });

static shared_pool_t shared_pool;

static allocator_t shared_test_allocator =
    INIT_ALLOCATOR(sizeof(short), ALLOC_N_BUCKETS, 1, false, &shared_pool);

#endif

void dispose_storage(allocator_t *alloc, size_t n, void *obj)
{
    size_t n_bucket;

    if (obj == NULL)
    {
        assert(n == 0);
        return;
    }

    assert(n > 0);
    n_bucket = alloc_calculate_bucket(alloc, n);

    if (n_bucket >= alloc->n_buckets)
    {
        assert(alloc->n_large_alloc > 0);
        alloc->n_large_alloc--;
        
        free(obj);
        return;
    }

    assert(alloc->buckets[n_bucket].n_alloc > 0);
    alloc->buckets[n_bucket].n_alloc--;
    
    n = alloc_bucket_size(alloc, n_bucket);
    if (alloc->pool != NULL &&
        pool_maybe_free(alloc->pool, obj, alloc->elt_size * n,
                        alloc->elt_size > sizeof(double) ?
                        sizeof(double) : alloc->elt_size))
    {
        return;
    }

    ((freelist_t *)obj)->chain = alloc->buckets[n_bucket].freelist;
    alloc->buckets[n_bucket].freelist = obj;
}

#if DO_TESTS

#define ASSERT_ALLOCATED(_alloc, _n, _expected)                         \
    ASSERT_EQ(unsigned,                                                 \
              (_n) < (_alloc).n_buckets ?                               \
              (_alloc).buckets[_n].n_alloc :                            \
              (_alloc).n_large_alloc, (_expected));

#define ASSERT_ALLOC_LIST(_alloc, _n, _expected)                        \
    do {                                                                \
        if ((_n) < (_alloc).n_buckets)                                  \
            ASSERT_EQ(ptr, (_alloc).buckets[_n].freelist, (_expected)); \
    } while(0)

TEST_SPEC(free_null, "Free NULL", false,
          {
              freelist_t *prev = test_allocator.buckets[0].freelist;
              dispose_storage(&test_allocator, 0, NULL);
              ASSERT_EQ(ptr, test_allocator.buckets[0].freelist, prev);
              ASSERT_EQ(unsigned, test_allocator.buckets[0].n_alloc, 0);
          });

TEST_SPEC(allocate_and_free,
          "Allocate and free an object", false,
          TEST_ITERATE(n, testval_small_uint_t,
                       {
                           void *objs = alloc_storage(&test_allocator,
                                                      n + 1);

                           ASSERT_NOT_NULL(objs);
                           ASSERT_ALLOCATED(test_allocator, n, 1);
                           
                           dispose_storage(&test_allocator, n + 1, objs);
                           ASSERT_ALLOCATED(test_allocator, n, 0);
                           ASSERT_ALLOC_LIST(test_allocator, n, (void *)objs);
                       }));

TEST_SPEC(allocate_free_allocate,
          "Allocate and free an object and allocate another", false,
          TEST_ITERATE(n, testval_small_uint_t,
                       {
                           if (n == 0)
                               continue;
                           else
                           {
                               void *objs = alloc_storage(&test_allocator, n);
                               void *objs1;
                               
                               dispose_storage(&test_allocator, n, objs);
                               objs1 = alloc_storage(&test_allocator, n);
                               ASSERT_EQ(ptr, objs, objs1);
                               ASSERT_NULL(test_allocator.buckets[n - 1].
                                           freelist);
                               ASSERT_EQ(unsigned,
                                         test_allocator.buckets[n - 1].
                                         n_alloc, 1);
                               dispose_storage(&test_allocator, n, objs1);
                               ASSERT_EQ(unsigned,
                                         test_allocator.buckets[n - 1].
                                         n_alloc, 0);
                           }
                       }
              ));

TEST_SPEC(alloc_from_pool,
          "Allocate from pool", false,
          {
              /* that's the space for arrays up to 10 short's +
               * double-boundary alignment
               */              
              shared_pool.available = 4 * sizeof(double) +
                  4 * 2 * sizeof(double) +
                  3 * sizeof(double);
              shared_pool.base = shared_pool.current = malloc(shared_pool.available);

              TEST_ITERATE(n, testval_small_uint_t,
                           {
                               short *obj = alloc_storage(&shared_test_allocator, n + 1);
                               void *pool_ptr = shared_pool.current;
                               
                               ASSERT_NOT_NULL(obj);
                               if (n < shared_test_allocator.n_buckets)
                               {
                                   ASSERT_EQ(ptr, obj, pool_ptr);
                                   ASSERT_EQ(unsigned,
                                             (uintptr_t)obj % sizeof(double), 0);
                               }
                               else
                               {
                                   ASSERT_NEQ(ptr, obj, pool_ptr);
                                   dispose_storage(&shared_test_allocator, n + 1, obj);
                               }
                           });
              ASSERT_EQ(unsigned, shared_pool.available, 0);
              free(shared_pool.base);
              shared_pool.base = NULL;
          });

#if 0
/** @testcase Allocate from pool and immediately free */
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

/** @testcase Malloc when no space in the pool */
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
#endif


#endif

void *grow_storage(allocator_t *alloc,
                   size_t * restrict n,
                   void * restrict * restrict objs,
                   size_t incr)
{
    size_t old_bucket;
    size_t new_bucket;

    assert(objs != NULL);
    

    if (*objs == NULL)
    {
        assert(*n == 0);
        *objs = alloc_storage(alloc, incr);
        *n = incr;
        return *objs;
    }
    if (incr == 0)
    {
        return (uint8_t *)*objs + *n * alloc->elt_size;
    }
    
    old_bucket = alloc_calculate_bucket(alloc, *n);
    new_bucket = alloc_calculate_bucket(alloc, *n + incr);

    assert(new_bucket >= old_bucket);
    if (old_bucket >= alloc->n_buckets || old_bucket != new_bucket)
    {
        void *new_objs = alloc_storage(alloc, *n + incr);

        memcpy(new_objs, *objs, *n * alloc->elt_size);
        dispose_storage(alloc, *n, *objs);
        *objs = new_objs;
    }

    *n += incr;
    return (uint8_t *)*objs + (*n - incr) * alloc->elt_size;
}

void *copy_storage(allocator_t *alloc, size_t n, const void *objs)
{
    void *result;
    
    if (objs == NULL || n == 0)
    {
        assert(n == 0);
        return NULL;
    }
    
    result = alloc_storage(alloc, n);
    memcpy(result, objs, n * alloc->elt_size);
    return result;
}

#if DO_TESTS
TEST_SPEC(copy, "Copy", false,
          TEST_ITERATE(n, testval_small_uint_t,
                       {
                           void *objs = alloc_storage(&test_allocator, n + 1);
                           void *objs1;

                           memset(objs, ARBITRARY(uint8_t, 0, UINT8_MAX), n);
                           objs1 = copy_storage(&test_allocator, n + 1, objs);
                           ASSERT_ALLOCATED(test_allocator, n, 2);
                           ASSERT_NOT_NULL(objs1);
                           ASSERT(memcmp(objs, objs1, n) == 0);
                           dispose_storage(&test_allocator, n + 1, objs);
                           dispose_storage(&test_allocator, n + 1, objs1);
                           ASSERT_ALLOCATED(test_allocator, n, 0);
                       }));

TEST_SPEC(deallocate_and_copy, 
          "Deallocate and copy", false,
          TEST_ITERATE(n, testval_small_uint_t,
                       {
                           if (n == TESTVAL_SMALL_UINT_MAX)
                               continue;
                           else
                           {
                               void *objs = alloc_storage(&test_allocator, n + 1);
                               void *objs1 = alloc_storage(&test_allocator, n + 1);
                               void *objs2 = NULL;
                               
                               dispose_storage(&test_allocator, n + 1, objs);
                               objs2 = copy_storage(&test_allocator, n + 1, objs1);
                               ASSERT_NEQ(ptr, objs2, objs1);
                               ASSERT_EQ(ptr, objs2, objs);
                               ASSERT_ALLOCATED(test_allocator, n, 2);
                               dispose_storage(&test_allocator, n + 1, objs1);
                               dispose_storage(&test_allocator, n + 1, objs2);
                               ASSERT_ALLOCATED(test_allocator, n, 0);
                           }
                       }));
#endif

void *append_storage(allocator_t *alloc,
                     size_t * restrict dest_sz, void ** restrict dest,
                     size_t app_sz, const void *app)
{
    void *tail;
    
    if (app_sz == 0)
    {
        return NULL;
    }
    
    assert(app != NULL);
    if (app == *dest)
    {
        assert(app_sz <= *dest_sz);
        tail = grow_storage(alloc, dest_sz, dest, app_sz);
        memcpy(tail, *dest, app_sz * alloc->elt_size);
    }
    else
    {
        tail = grow_storage(alloc, dest_sz, dest, app_sz);
        memcpy(tail, app, app_sz * alloc->elt_size);
    }
    return tail;
}

#if DO_TESTS

RUN_TESTSUITE("Allocator");


#endif
