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
 * @brief common test definitions
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef TESTS_H
#define TESTS_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <CUnit/CUnit.h>

typedef struct test_descr {
    const char *name;
    CU_TestFunc test;
} test_descr;

#define TEST_DESCR(_test) { #_test, test_##_test }    

typedef struct test_suite_descr {
    const char *name;
    CU_InitializeFunc init;
    CU_CleanupFunc cleanup;
    const test_descr *tests;
} test_suite_descr;

extern test_suite_descr support_tests;
extern test_suite_descr allocator_tests;
extern test_suite_descr stack_tests;
extern test_suite_descr queue_tests;


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* TESTS_H */
