/*
 * Copyright (c) 2015  Artem V. Andreev
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
/**
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */

#include <CUnit/CUnit.h>
#include "tests.h"
#include "support.h"

static void test_clz(void) 
{
    CU_ASSERT_EQUAL(count_leading_zeroes(0u), sizeof(unsigned) * CHAR_BIT);
    CU_ASSERT_EQUAL(count_leading_zeroes(1u), sizeof(unsigned) * CHAR_BIT - 1);
    CU_ASSERT_EQUAL(count_leading_zeroes(0x12340u), sizeof(unsigned) * CHAR_BIT - 17);
    CU_ASSERT_EQUAL(count_leading_zeroes(UINT_MAX), 0);
    CU_ASSERT_EQUAL(count_leading_zeroes(UINT_MAX >> 1), 1);
    CU_ASSERT_EQUAL(count_leading_zeroes(UINT_MAX >> 2), 2);
}

test_suite_descr support_tests = {
    "support", NULL, NULL,
    (const test_descr[]){
        TEST_DESCR(clz),
        {NULL, NULL}
    }
};
