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
/** @file
 * @brief testing a test framework
 * 
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 * @test Background:
 * @code
 * #define DO_TESTING 1
 * @endcode
 */
#ifndef SELFTEST_H
#define SELFTEST_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(DO_TESTING) && !defined(__DOXYGEN__)
#error "This file should never be included in real sources!"
#endif

/**
 * Test data
 * @test Background:
 * @code
 * @endcode
 */
struct test_data {
};

/**
 * Test function 1
 * @test Simple Test
 * - Given a suitable environment: `unsigned state = 0;`
 * - When a certain condition is met: `state++;`
 * - Then some assertion holds true: `ASSERT_UINT_EQ(state, 1);`
 * - And some other assertion also holds: `ASSERT_UINT_NEQ(state, 0);`
 * - But other issues shall be taken into account: `ASSERT(1);`
 * - Cleanup: `state = 0;`
 *
 * @test Compound test
 * - Given a suitable environment: `unsigned state = 0;`
 * - And given some change in the environment: `state++;`
 * - When the state is initial: `if (state == 0)`
 *   + Then nothing is good: `ASSERT(0);`
 * - When the state is not initial: `if (state > 0)`
 *   + Verify that the state is consistent: `ASSERT_UINT_EQ(state, 1);`
 *   + When the state is changed again: `state++;`
 *   + Then some other conditions are met: `ASSERT_UINT_EQ(state, 2);`
 *
 * @test 
 * Verify that the expected value is correct: `ASSERT_UINT_EQ(input + 1, expected);`
 * @testvar{unsigned,input,u} | @testvar{unsigned,expected}
 * ---------------------------|----------------------------
 * `0`                        | `1`
 * `1`                        | `2`
 * `100`                      | `101`
 *
 * @test 
 * Verify that the expected value is correct: `ASSERT_UINT_GREATER(input, 0);`
 * Varying                    |From | To 
 * ---------------------------|-----|----
 * @testvar{unsigned,input,u} | `1` | `5`
 *
 * @test Multiparameterized
 * Varying                    |From | To 
 * ---------------------------|-----|----
 * @testvar{unsigned,i,u}     | `0` | `3`
 * @li Given a certain state `unsigned state = i;`
 * @li Verify that something goes on as expected: `ASSERT_UINT_EQ(j + k, expected);`
 *   |@testvar{unsigned,j,u}|
 *   |----------------------|
 *   |`0`                   |
 *   |`1`                   |
 *   |`2`                   |
 *   @testvar{unsigned,k,u}|@testvar{unsigned,expected,u}
 *   ----------------------|---------------------------
 *   `state`               |`j + i`
 *   `state + 1`           |`j + i + 1`
 */
extern void testfunc1(void);

/**
 * Test function 2
 * @test Background:
 * - Given a certain state: `unsigned state = 0;`
 * - When something happens: `state++;`
 *
 * @test Test with Background
 * - Then the state is correct: `ASSERT_UINT_EQ(state, 1)`;
 */
extern void testfunc2(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SELFTEST_H */
