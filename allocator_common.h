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

typedef struct freelist_t {
    struct freelist_t * chain;
} freelist_t;
  
static inline
returns(fresh_pointer) returns(not_null) returns(important)
void *frlmalloc(size_t sz) 
{
    void *result = malloc(sz > sizeof(freelist_t) ? sz : sizeof(freelist_t));
    assert(result != NULL);
    return result;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ALLOCATOR_COMMON_H */
