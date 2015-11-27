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

#if NONFATAL_ASSERTIONS
unsigned assert_failure_count = 0;
#endif

extern const char testsuite_descr[];
extern void run_tests(void);

#if NONFATAL_ASSERTIONS
static unsigned current_failure_count = 0;
#endif

extern void do_start_test(const char *descr);
void do_start_test(const char *descr)
{
#if NONFATAL_ASSERTIONS
    current_failure_count = assert_failure_count;
#endif
    fprintf(stderr, "%s... ", descr);
    fflush(stderr);
}

extern void do_end_test(void);
void do_end_test(void)
{
#if NONFATAL_ASSERTIONS
    fputs(current_failure_count < assert_failure_count ?
          "FAIL\n" : "OK\n", stderr);
#else        
    fputs("OK\n", stderr);
#endif
}

int main(void)
{
    SET_RANDOM_SEED();
    fprintf(stderr, "%s:\n", testsuite_descr);
    run_tests();

#if NONFATAL_ASSERTIONS
    if (assert_failure_count > 0)
    {
        fprintf(stderr, "%u assertions FAILED\n", assert_failure_count);
        return 1;
    }
#endif
    return 0;
}
