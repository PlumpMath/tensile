/****h* Library/Bitops
 * COPYRIGHT 
 * (c) 2016  Artem V. Andreev
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
 *
 * NAME
 * bitops.c --- Bit operatons
 *
 * AUTHOR 
 * Artem V. Andreev <artem@AA5779.spb.edu>
 *****/

/* public */
#include "compiler.h"
#include "bitops_api.h"


/****f* Bitops/count_leading_zeroes
 * NAME
 * count_leading_zeroes --- Count leading zeroes
 *
 * RETURN VALUE
 * The number of upper zero bits before the first 1
 ******/
/* public */
static inline unsigned count_leading_zeroes(size_t i)
{
#if __GNUC__ >= 4
    if (i == 0u)
        return sizeof(i) * CHAR_BIT;
    return (unsigned)__builtin_clzl(i);
#else
    unsigned j;
    
    if (i == 0)
        return sizeof(i) * CHAR_BIT;
    for (j = sizeof(i) * CHAR_BIT - 1; j > 0; j--)
    {
        if ((i & (1ul << j)) != 0)
            return (unsigned)sizeof(i) * CHAR_BIT - 1 - j;
    }
    return sizeof(i) * CHAR_BIT - 1;
#endif
}

/* TESTS */
#include "assertions.h"
#include "bitops_impl.c"
#define TESTSUITE "Bit operations"

/* testcase: Zero */
static void test_zero(void)
{
    ASSERT_EQ(unsigned, count_leading_zeroes(0), sizeof(size_t) * CHAR_BIT);
}

/* testcase: All ones */
static void test_all_ones(void)
{
    ASSERT_EQ(unsigned, count_leading_zeroes(SIZE_MAX), 0);
}

/* testcase: Arbitrary bit */
static void test_one_bit(testval_bitnum_size_t bit)
{
    ASSERT_EQ(unsigned,
              count_leading_zeroes(1UL << bit),
              sizeof(size_t) * CHAR_BIT - bit - 1);
    ASSERT_EQ(unsigned,
              count_leading_zeroes((1UL << bit) | 1),
              sizeof(size_t) * CHAR_BIT - bit - 1);
}

/* END */
