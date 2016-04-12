/*
 * Copyright (c) 2016  Artem V. Andreev
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
 */
/** @file
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */

#include "bitops.h"
#include "assertions.h"

TEST_SPEC(test_zero, "Count leading zeroes in 0", false,
{
    ASSERT_EQ(unsigned, count_leading_zeroes(0), sizeof(size_t) * CHAR_BIT);
});

TEST_SPEC(test_all_ones, "Count leading zeroes in all ones", false,
          {
              ASSERT_EQ(unsigned, count_leading_zeroes(SIZE_MAX), 0);
          });

TEST_SPEC(test_one_bit, "Leading zeroes for a single bit", false,
          TEST_ITERATE(bit, testval_bitnum_size_t,
                       {
                           ASSERT_EQ(unsigned,
                                     count_leading_zeroes(1UL << bit),
                                     sizeof(size_t) * CHAR_BIT - bit - 1);
                           ASSERT_EQ(unsigned,
                                     count_leading_zeroes((1UL << bit) | 1),
                                     sizeof(size_t) * CHAR_BIT - bit - 1);
                       }));

RUN_TESTSUITE("Bit operations");

