/* 
 * Copyright (c) 2015-2016  Artem V. Andreev
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
 * @brief @internal Assertion macros
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef ASSERTIONS_H
#define ASSERTIONS_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

typedef struct testrun {
    unsigned n_samples;
    bool verbose;
} testrun;

typedef void (*testable_method_gen)(void *, unsigned *);
typedef bool (*testable_method_end)(void *, const unsigned *);

typedef bool (*testable_method_cmp)(const void *, const void *);

typedef bool (*testable_method_pred)(const void *);

typedef void (*testable_method_log)(FILE *, const void *);

typedef struct testable {
    testable_method_gen init;
    testable_method_gen next;
    testable_method_end  end;
    testable_method_cmp  eq;
    testable_method_cmp  neq;
    testable_method_cmp  less;
    testable_method_cmp  gt;
    testable_method_cmp  le;
    testable_method_cmp  ge;
    testable_method_pred isnull;
    testable_method_log  log;
} testable;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ASSERTIONS_H */
