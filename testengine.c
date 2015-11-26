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
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */

#include "assertions.h"

extern const char testsuite_descr[];
extern void run_tests(void);

int main(void)
{
    unsigned i;
    
    SET_RANDOM_SEED();
    fprintf(stderr, "%s:\n", TESTSUITE);
    for (i = 0; test_cases[i].action != NULL; i++)
    {
#if NONFATAL_ASSERTIONS
        unsigned current_failure_count = assert_failure_count;
#endif
        fprintf(stderr, "%s... ", test_cases[i].descr);
        fflush(stderr);
        test_cases[i].action();
#if NONFATAL_ASSERTIONS
        fputs(current_failure_count < assert_failure_count ?
              "FAIL\n" : "OK\n", stderr);
#else        
        fputs("OK\n", stderr);
#endif        
    }
#if NONFATAL_ASSERTIONS
    if (assert_failure_count > 0)
    {
        fprintf(stderr, "%u assertions FAILED\n", assert_failure_count);
        return 1;
    }
#endif
    return 0;
}
