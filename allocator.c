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

annotation(global_state, none) annotation(returns, important)
static size_t object_alignment(size_t objsize)
{
    size_t align = objsize & (-objsize);

    return align > sizeof(double) ? sizeof(double) : align;
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
                             object_alignment(alloc->elt_size));
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

typedef unsigned testval_alloc_size_t;

#define TESTVAL_GENERATE__testval_alloc_size_t                          \
    TESTVAL_GENERATE_ARBITRARY(testval_alloc_size_t, 1,                 \
                               sizeof(void *) - 1),                     \
        sizeof(void *),                                                 \
        TESTVAL_GENERATE_ARBITRARY(testval_alloc_size_t,                \
                                   sizeof(double) + 1, 2 * sizeof(double))

#define TESTVAL_LOG_FMT_testval_alloc_size_t "%u"
#define TESTVAL_LOG_ARGS_testval_alloc_size_t(_x) (_x)

#define ITERATE_ALLOCATOR(_size, _alloc, _bsize, _log, _n, _body)       \
    TEST_ITERATE(_size, testval_alloc_size_t,                           \
                 {                                                      \
                     allocator_t _alloc =                               \
                         INIT_ALLOCATOR(_size, ALLOC_N_BUCKETS,         \
                                        _bsize, _log, NULL);            \
                     TEST_ITERATE(_n, testval_small_uint_t,             \
                                  _body);                               \
                     cleanup_allocator(&_alloc);                        \
                 })

#define ITERATE_ALLOCATOR_POOL(_available, _pool,                       \
                               _size, _alloc, _bsize, _log,             \
                               _initpool,                               \
                               _n, _body, _finipool)                    \
    TEST_ITERATE(_size, testval_alloc_size_t,                           \
                 {                                                      \
                     size_t _available = 0;                             \
                     shared_pool_t _pool;                               \
                     allocator_t _alloc =                               \
                         INIT_ALLOCATOR(_size, ALLOC_N_BUCKETS,         \
                                        _bsize, _log, &_pool);          \
                     _initpool;                                         \
                     shared_pool_init(&_pool, _available);              \
                     TEST_ITERATE(_n, testval_small_uint_t,             \
                                  _body);                               \
                     _finipool;                                         \
                     cleanup_allocator(&_alloc);                        \
                     free(_pool.base);                                  \
                 })

#define ALLOC_N_BUCKETS (TESTVAL_SMALL_UINT_MAX + 1)

TEST_SPEC(object_align, "Object alignment", false,
          TEST_ITERATE(n, testval_alloc_size_t,
                       {
                           if (n % sizeof(double) == 0)
                               ASSERT_EQ(unsigned, object_alignment(n), sizeof(double));
                           else if (n % sizeof(unsigned) == 0)
                               ASSERT_EQ(unsigned, object_alignment(n), sizeof(unsigned));
                           else if (n % sizeof(short) == 0)
                               ASSERT_EQ(unsigned, object_alignment(n), sizeof(short));
                           else
                               ASSERT_EQ(unsigned, object_alignment(n), 1);
                       }));


TEST_SPEC(initialize_allocator,
          "Allocator properly initialized", false,
          {
              allocator_t allocator =
                  INIT_ALLOCATOR(1, ALLOC_N_BUCKETS, 1, false, NULL);
              unsigned i;

              ASSERT_EQ(unsigned, allocator.n_buckets, ALLOC_N_BUCKETS);
              for (i = 0; i < allocator.n_buckets; i++)
              {
                  ASSERT_NULL(allocator.buckets[i].freelist);
                  ASSERT_EQ(unsigned, allocator.buckets[i].n_alloc, 0);
              }
              ASSERT_EQ(unsigned, allocator.n_large_alloc, 0);
          });

TEST_SPEC(allocate_zero, "Allocate zero-sized array", false,
          {
              allocator_t allocator =
                  INIT_ALLOCATOR(1, ALLOC_N_BUCKETS, 1, false, NULL);
              ASSERT_NULL(alloc_storage(&allocator, 0));
              ASSERT_EQ(unsigned, allocator.buckets[0].n_alloc, 0);
          });


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
                        object_alignment(alloc->elt_size)))
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
              allocator_t allocator =
                  INIT_ALLOCATOR(1, ALLOC_N_BUCKETS, 1, false, NULL);

              dispose_storage(&allocator, 0, NULL);
              ASSERT_NULL(allocator.buckets[0].freelist);
              ASSERT_EQ(unsigned, allocator.buckets[0].n_alloc, 0);
          });

TEST_SPEC(allocate_and_free,
          "Allocate and free an object", false,
          ITERATE_ALLOCATOR(size, alloc, 1, false, n,
                            {
                                void *objs = alloc_storage(&alloc, n + 1);
                                
                                ASSERT_NOT_NULL(objs);
                                ASSERT_ALLOCATED(alloc, n, 1);
                                
                                dispose_storage(&alloc, n + 1, objs);
                                ASSERT_ALLOCATED(alloc, n, 0);
                                ASSERT_ALLOC_LIST(alloc, n, (void *)objs);
                            }));

TEST_SPEC(allocate_free_allocate,
          "Allocate and free an object and allocate another", false,
          ITERATE_ALLOCATOR(size, alloc, 1, false, n,
                            {
                                if (n == 0)
                                    continue;
                                else
                                {
                                    void *objs = alloc_storage(&alloc, n);
                                    void *objs1;
                                    
                                    dispose_storage(&alloc, n, objs);
                                    objs1 = alloc_storage(&alloc, n);
                                    ASSERT_EQ(ptr, objs, objs1);
                                    ASSERT_NULL(alloc.buckets[n - 1].freelist);
                                    ASSERT_EQ(unsigned,
                                              alloc.buckets[n - 1].n_alloc, 1);
                                    dispose_storage(&alloc, n, objs1);
                                    ASSERT_EQ(unsigned,
                                              alloc.buckets[n - 1].n_alloc, 0);
                                }
                            }));

TEST_SPEC(allocate_free_allocate2,
          "Allocate and free an object and allocate two more", false,
          ITERATE_ALLOCATOR(size, alloc, 1, false, n,
                            {
                                void *obj = alloc_storage(&alloc, n + 1);
                                void *obj1;
                                void *obj2;
                                
                                dispose_storage(&alloc, n + 1, obj);
                                obj1 = alloc_storage(&alloc, n + 1);
                                
                                if (n < TESTVAL_SMALL_UINT_MAX)
                                    ASSERT_EQ(ptr, obj1, obj);

                                obj2 = alloc_storage(&alloc, n + 1);
                                ASSERT_NEQ(ptr, obj2, obj1);
                                ASSERT_ALLOCATED(alloc, n, 2);
                                dispose_storage(&alloc, n + 1, obj1);
                                dispose_storage(&alloc, n + 1, obj2);
                                ASSERT_ALLOCATED(alloc, n, 0);
                            }));

TEST_SPEC(alloc_from_pool,
          "Allocate from pool", false,
          ITERATE_ALLOCATOR_POOL(available, pool, size, alloc, 1, false,
                                 size_t align = object_alignment(size);
                                 unsigned i;

                                 for (i = 1; i < alloc.n_buckets; i++)
                                 {
                                     available +=
                                         (i * alloc.elt_size + align - 1) / align *
                                         align;
                                 },
                                 n, 
                                 {
                                     void *pool_ptr = pool.current;                               
                                     void *obj = alloc_storage(&alloc, n + 1);
                                     
                                     ASSERT_NOT_NULL(obj);
                                     ASSERT_ALLOCATED(alloc, n, 1);
                                     if (n + 1 < alloc.n_buckets)
                                     {
                                         ASSERT_EQ(unsigned,
                                                   (uintptr_t)obj % align, 0);                                         
                                         ASSERT_LESS(unsigned,
                                                     (uintptr_t)obj -  (uintptr_t)pool_ptr,
                                                     align);
                                         alloc.buckets[n].n_alloc = 0;
                                     }
                                     else
                                     {
                                         ASSERT_NEQ(ptr, obj, pool_ptr);
                                         dispose_storage(&alloc, n + 1, obj);
                                     }
                                 },
                                 ASSERT_EQ(unsigned, pool.available, 0)));

TEST_SPEC(alloc_from_pool_and_return,
          "Allocate from pool and immediately free", false,
          ITERATE_ALLOCATOR_POOL(available, pool, size, alloc, 1, false,
                                 available = alloc.n_buckets * alloc.elt_size,
                                 n,
                                 {
                                     void *obj = alloc_storage(&alloc, n + 1);
                                     
                                     if (n < alloc.n_buckets)
                                     {
                                         ASSERT_EQ(ptr, obj, pool.base);
                                     }
                                     else
                                     {
                                         ASSERT_NEQ(ptr, obj, pool.base);
                                     }
                                     
                                     dispose_storage(&alloc, n + 1, obj);
                                     ASSERT_ALLOCATED(alloc, n, 0);
                                     ASSERT_EQ(unsigned, available, pool.available);
                                     ASSERT_ALLOC_LIST(alloc, n, NULL);
                                 }, /* nothing */));

TEST_SPEC(alloc_from_pool_and_reverse_free,
          "Allocate from pool and free in reverse order", false,
          ITERATE_ALLOCATOR_POOL(available, pool, size, alloc, 1, false,
                                 available = 2 * alloc.n_buckets * alloc.elt_size,
                                 n,
                                 {
                                     void *obj;
                                     void *obj1;
                                     void *current;
                                     size_t cursize;
                                     
                                     if (n == 0)
                                         continue;
                                     
                                     obj = alloc_storage(&alloc, n);
                                     obj1 = alloc_storage(&alloc, n);
                                     
                                     ASSERT_EQ(ptr, obj, pool.base);
                                     current = pool.current;
                                     cursize = pool.available;
                                     
                                     dispose_storage(&alloc, n, obj);                                            
                                     ASSERT_EQ(unsigned, pool.available, cursize);
                                     ASSERT_EQ(ptr, pool.current, current);
                                     ASSERT_ALLOC_LIST(alloc, n - 1, obj);
                                     dispose_storage(&alloc, n, obj1);
                                     ASSERT_ALLOCATED(alloc, n - 1, 0);
                                     ASSERT_ALLOC_LIST(alloc, n - 1, obj);
                                     ASSERT_EQ(ptr, pool.current,
                                               (uint8_t *)pool.base + n * alloc.elt_size);
                                     ASSERT_EQ(unsigned, pool.available,
                                               2 * alloc.n_buckets * alloc.elt_size -
                                               (size_t)((uint8_t *)pool.current - (uint8_t *)pool.base));
                                     alloc.buckets[n - 1].freelist = NULL;
                                     pool.current = pool.base;
                                     pool.available = available;
                                 }, /* nothing */));

TEST_SPEC(alloc_not_from_pool,
          "Malloc when no space in the pool", false,
          TEST_ITERATE(size, testval_alloc_size_t,
                       TEST_ITERATE(n, testval_small_uint_t,
                                    {
                                        shared_pool_t pool;
                                        size_t available = (n + 1) * size - 1;
                                        allocator_t alloc =
                                            INIT_ALLOCATOR(size, ALLOC_N_BUCKETS, 1, false, &pool);
                                        void *obj;
                                        
                                        shared_pool_init(&pool, available);
                                        obj = alloc_storage(&alloc, n + 1);
                                        
                                        ASSERT_NOT_NULL(obj);
                                        ASSERT_NEQ(ptr, obj, pool.base);
                                        ASSERT_EQ(ptr, pool.base, pool.current);
                                        ASSERT_EQ(unsigned, pool.available, available);
                                        
                                        dispose_storage(&alloc, n + 1, obj);
                                        ASSERT_EQ(ptr, pool.base, pool.current);
                                        ASSERT_EQ(unsigned, pool.available, available);
                                        
                                        ASSERT_ALLOC_LIST(alloc, n, obj);
                                        free(pool.base);
                                        cleanup_allocator(&alloc);
                                    })));

TEST_SPEC(alloc_from_pool_mixed,
          "Alloc from pool using different allocators", false,
          {
              shared_pool_t pool;
              unsigned small = ARBITRARY(unsigned, 1, sizeof(double) - 1);
              unsigned large = ARBITRARY(unsigned, sizeof(double), 2 * sizeof(double));
              allocator_t small_alloc =
                  INIT_ALLOCATOR(small, ALLOC_N_BUCKETS, 1, false, &pool);
              allocator_t large_alloc =
                  INIT_ALLOCATOR(large, ALLOC_N_BUCKETS, 1, false, &pool);              
              void *obj1;
              void *obj2;
              void *obj3;
              size_t small_align = object_alignment(small);
              size_t large_align = object_alignment(large);
              
              shared_pool_init(&pool,
                               2 * large_alloc.elt_size + sizeof(double));
              obj1 = alloc_storage(&large_alloc, 1);
              ASSERT_EQ(ptr, obj1, pool.base);
              obj2 = alloc_storage(&small_alloc, 1);
              ASSERT_EQ(ptr, obj2, (uint8_t *)pool.base +
                        (large_alloc.elt_size + small_align - 1) /
                        small_align * small_align);
              obj3 = alloc_storage(&large_alloc, 1);
              ASSERT_EQ(ptr, obj3,
                        (uint8_t *)pool.base +
                        ((uintptr_t)((uint8_t *)obj2 + small_alloc.elt_size -
                                     (uint8_t *)pool.base) +
                         large_align - 1) / large_align * large_align);
              dispose_storage(&large_alloc, 1, obj3);
              ASSERT_EQ(ptr, pool.current, obj3);              
              dispose_storage(&small_alloc, 1, obj2);
              dispose_storage(&large_alloc, 1, obj1);
              
              free(pool.base);
          });

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
          ITERATE_ALLOCATOR(size, alloc, 1, false, n,
                            {
                                void *objs = alloc_storage(&alloc, n + 1);
                                void *objs1;

                                memset(objs, ARBITRARY(uint8_t, 1, UINT8_MAX), n);
                                objs1 = copy_storage(&alloc, n + 1, objs);
                                ASSERT_ALLOCATED(alloc, n, 2);
                                ASSERT_NOT_NULL(objs1);
                                ASSERT(memcmp(objs, objs1, n) == 0);
                                dispose_storage(&alloc, n + 1, objs);
                                dispose_storage(&alloc, n + 1, objs1);
                                ASSERT_ALLOCATED(alloc, n, 0);
                            }));

TEST_SPEC(deallocate_and_copy, 
          "Deallocate and copy", false,
          ITERATE_ALLOCATOR(size, alloc, 1, false, n,          
                            {
                                if (n == TESTVAL_SMALL_UINT_MAX)
                                    continue;
                                else
                                {
                                    void *objs = alloc_storage(&alloc, n + 1);
                                    void *objs1 = alloc_storage(&alloc, n + 1);
                                    void *objs2 = NULL;
                               
                                    dispose_storage(&alloc, n + 1, objs);
                                    objs2 = copy_storage(&alloc, n + 1, objs1);
                                    ASSERT_NEQ(ptr, objs2, objs1);
                                    ASSERT_EQ(ptr, objs2, objs);
                                    ASSERT_ALLOCATED(alloc, n, 2);
                                    dispose_storage(&alloc, n + 1, objs1);
                                    dispose_storage(&alloc, n + 1, objs2);
                                    ASSERT_ALLOCATED(alloc, n, 0);
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

void cleanup_allocator(allocator_t *alloc)
{
    unsigned i;

    assert(alloc->n_large_alloc == 0);
    for (i = 0; i < alloc->n_buckets; i++)
    {
        freelist_t *iter, *next;

        assert(alloc->buckets[i].n_alloc == 0);
        for (iter = alloc->buckets[i].freelist; iter != NULL; iter = next)
        {
            next = iter->chain;
            free(iter);
        }
        alloc->buckets[i].freelist = NULL;        
    }
}
          
#if DO_TESTS

RUN_TESTSUITE("Allocator");


#endif
