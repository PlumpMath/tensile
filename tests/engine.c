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

#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>
#include "tests.h"
#include "support.h"

static void add_test_suite(const test_suite_descr *suite_descr) 
{
    CU_pSuite suite = CU_add_suite(suite_descr->name,
                                   suite_descr->init,
                                   suite_descr->cleanup);
    const test_descr *test;
    
    if (suite == NULL) 
    {
        fprintf(stderr, "Cannot add suite '%s'\n", suite_descr->name);
        exit(EXIT_FAILURE);
    }

    for (test = suite_descr->tests; test->name != NULL; test++) 
    {
        if (CU_add_test(suite, test->name, test->test) == NULL) 
        {
            fprintf(stderr, "Cannot add test '%s' to suite '%s'\n",
                    test->name, suite_descr->name);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc ATTR_UNUSED, char *argv[] ATTR_UNUSED)
{
    if (CU_initialize_registry() != CUE_SUCCESS) 
    {
        fputs("Cannot initialize CUnit!\n", stderr);
        exit(EXIT_FAILURE);
    }

    add_test_suite(&allocator_tests);
    
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}
