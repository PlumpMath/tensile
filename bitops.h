/* 
 * Copyright (c) 2016  Artem V. Andreev
 *
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
 * @brief Bitwise operations
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef BITOPS_H
#define BITOPS_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include "compiler.h"


/** Count leading zeroes
 * @return The number of upper zero bits before the first 1
 */
annotation(global_state, none)
static inline unsigned count_leading_zeroes(size_t i)
{
#if __GNUC__ >= 4
    if (i == 0u) {
        return sizeof(i) * CHAR_BIT;
    }
    return (unsigned)__builtin_clzl(i);
#else
    unsigned j;

    if (i == 0)
    {
        return sizeof(i) * CHAR_BIT;
    }
    for (j = sizeof(i) * CHAR_BIT - 1; j > 0; j--)
    {
        if ((i & (1ul << j)) != 0)
        {
            return (unsigned)sizeof(i) * CHAR_BIT - 1 - j;
        }
    }
    return sizeof(i) * CHAR_BIT - 1;
#endif
}

#ifdef __cplusplus
}
#endif
#endif /* BITOPS_H */
