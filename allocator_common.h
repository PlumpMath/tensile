/****ih Library/AllocatorInternal
 * COPYRIGHT
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
 *
 * NAME
 * allocator_common.h --- Type-independent internal allocator code
 *
 * AUTHOR
 * Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef ALLOCATOR_COMMON_H
#define ALLOCATOR_COMMON_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include "compiler.h"
#include "bitops_api.h"

typedef struct freelist_t {
    struct freelist_t * chain;
} freelist_t;

static inline
returns(fresh_pointer) returns(not_null) returns(important)
void *frlmalloc(size_t sz)
{
    void *result = malloc(sz > sizeof(freelist_t) ? sz : sizeof(freelist_t));
    if (result == NULL)
        abort();
    
    return result;
}

static inline global_state(none) returns(important)
size_t pool_size_aligned(size_t sz, size_t align)
{
    return (sz + align - 1) / align * align;
}

static inline
returns(important) arguments(not_null) argument(storage_size,3)
void *pool_alloc(void * restrict * restrict pool,
                 size_t * restrict pool_size,
                 size_t objsize, size_t align)
{
    size_t alloc_size = pool_size_aligned(objsize, align);
    void *result;

    if (*pool_size < alloc_size)
        return NULL;
    result = *pool;
    *pool = (uint8_t *)*pool + alloc_size;
    *pool_size -= alloc_size;

    return result;
}

static inline arguments(not_null) returns(important)
bool pool_maybe_free(void * restrict * restrict pool,
                     size_t * restrict pool_size,
                     void *obj, size_t objsize, size_t align)
{
    size_t alloc_size = pool_size_aligned(objsize, align);

    if (*pool == (uint8_t *)obj + alloc_size)
    {
        *pool = obj;
        *pool_size += alloc_size;
        return true;
    }
    return false;
}

static inline global_state(none) returns(important)
size_t alloc_calculate_bucket(size_t n, size_t bucket_size, bool log_scale)
{
    assert(n > 0);
    n = (n - 1) / bucket_size;
    if (log_scale)
        n = sizeof(n) * CHAR_BIT - count_leading_zeroes(n);
    return n;
}

static inline global_state(none) returns(important)
size_t alloc_bucket_size(size_t n_bucket, size_t bucket_size, bool log_scale)
{
    if (log_scale)
        n_bucket = 1u << n_bucket;
    else
        n_bucket++;
    return n_bucket * bucket_size;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ALLOCATOR_COMMON_H */
