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
 * @brief auxiliary assertion macros
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef ASSERTIONS_H
#define ASSERTIONS_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#if !NONFATAL_ASSERTIONS
#define ASSERT_FAIL abort()
#else
extern unsigned assert_failure_count;
#define ASSERT_FAIL (assert_failure_count++)
#endif

#define ASSERT(_expr)                                                   \
    do {                                                                \
        if (!(_expr))                                                   \
        {                                                               \
            fprintf(stderr,                                             \
                    "Assertion " #_expr " failed at %s[%s:%u]\n",       \
                    __FUNCTION__, __FILE__, __LINE__);                  \
            ASSERT_FAIL;                                                \
        }                                                               \
    } while(0)

#define ASSERT_NOT(_cond) ASSERT(!(_cond))

#define __ASSERT2(_type, _fmt, _expr1, _comparator, _cmpname, _expr2)   \
    do {                                                                \
        _type __var1 = _expr1;                                          \
        _type __var2 = _expr2;                                          \
                                                                        \
        if (!_comparator(__var1, __var2))                               \
        {                                                               \
            fprintf(stderr,                                             \
                    "Assertion " #_expr1 " " _cmpname " " #_expr2       \
                    " failed at %s[%s:%u]: expected " _fmt              \
                    ", got " _fmt "\n",                                 \
                    __FUNCTION__, __FILE__, __LINE__,                   \
                    __var1, __var2);                                    \
            ASSERT_FAIL;                                                \
        }                                                               \
    } while(0)

#define _ASSERT2(_type, _expr1, _cmpid, _expr2)         \
    __ASSERT2(ASSERT_TYPE_##_type,                      \
              ASSERT_FMT_##_type,                       \
              _expr1,                                   \
              ASSERT_TYPE_CMP_##_type##_##_cmpid,       \
              ASSERT_CMP_NAME_##_cmpid,                 \
              _expr2)

#define ASSERT_TYPE_int      long
#define ASSERT_TYPE_unsigned unsigned long
#define ASSERT_TYPE_float    double
#define ASSERT_TYPE_string   const char *
#define ASSERT_TYPE_wstring  const wchar_t *
#define ASSERT_TYPE_ptr      const void *
#define ASSERT_TYPE_bits     unsigned long long

#define ASSERT_FMT_int "%ld"
#define ASSERT_FMT_unsigned "%lu"
#define ASSERT_FMT_float "%g"
#define ASSERT_FMT_string "%s"
#define ASSERT_FMT_wstring "%ls"
#define ASSERT_FMT_ptr "%p"
#define ASSERT_FMT_bits "%llx"

#define ASSERT_CMP_NAME_eq "=="
#define ASSERT_CMP_NAME_neq "!="
#define ASSERT_CMP_NAME_less "<"
#define ASSERT_CMP_NAME_greater ">"
#define ASSERT_CMP_NAME_le "<="
#define ASSERT_CMP_NAME_ge ">="

#define ASSERT_TYPE_CMP_int_eq(_x1, _x2) ((_x1) == (_x2))
#define ASSERT_TYPE_CMP_int_neq(_x1, _x2) ((_x1) != (_x2))
#define ASSERT_TYPE_CMP_int_less(_x1, _x2) ((_x1) < (_x2))
#define ASSERT_TYPE_CMP_int_greater(_x1, _x2) ((_x1) > (_x2))
#define ASSERT_TYPE_CMP_int_le(_x1, _x2) ((_x1) <= (_x2))
#define ASSERT_TYPE_CMP_int_ge(_x1, _x2) ((_x1) >= (_x2))

#define ASSERT_TYPE_CMP_unsigned_eq(_x1, _x2) ((_x1) == (_x2))
#define ASSERT_TYPE_CMP_unsigned_neq(_x1, _x2) ((_x1) != (_x2))
#define ASSERT_TYPE_CMP_unsigned_less(_x1, _x2) ((_x1) < (_x2))
#define ASSERT_TYPE_CMP_unsigned_greater(_x1, _x2) ((_x1) > (_x2))
#define ASSERT_TYPE_CMP_unsigned_le(_x1, _x2) ((_x1) <= (_x2))
#define ASSERT_TYPE_CMP_unsigned_ge(_x1, _x2) ((_x1) >= (_x2))

#define ASSERT_TYPE_CMP_float_eq(_x1, _x2) ((_x1) == (_x2))
#define ASSERT_TYPE_CMP_float_neq(_x1, _x2) ((_x1) != (_x2))
#define ASSERT_TYPE_CMP_float_less(_x1, _x2) ((_x1) < (_x2))
#define ASSERT_TYPE_CMP_float_greater(_x1, _x2) ((_x1) > (_x2))
#define ASSERT_TYPE_CMP_float_le(_x1, _x2) ((_x1) <= (_x2))
#define ASSERT_TYPE_CMP_float_ge(_x1, _x2) ((_x1) >= (_x2))    

#define ASSERT_TYPE_CMP_string_eq(_x1, _x2) (strcmp(_x1, _x2) == 0)
#define ASSERT_TYPE_CMP_string_neq(_x1, _x2) (strcmp(_x1, _x2) != 0)
#define ASSERT_TYPE_CMP_string_less(_x1, _x2) (strcmp(_x1, _x2) < 0)
#define ASSERT_TYPE_CMP_string_greater(_x1, _x2) (strcmp(_x1, _x2) > 0)
#define ASSERT_TYPE_CMP_string_le(_x1, _x2) (strcmp(_x1, _x2) <= 0)
#define ASSERT_TYPE_CMP_string_ge(_x1, _x2) (strcmp(_x1, _x2) >= 0)

#define ASSERT_TYPE_CMP_wstring_eq(_x1, _x2) (wcscmp(_x1, _x2) == 0)
#define ASSERT_TYPE_CMP_wstring_neq(_x1, _x2) (wcscmp(_x1, _x2) != 0)
#define ASSERT_TYPE_CMP_wstring_less(_x1, _x2) (wcscmp(_x1, _x2) < 0)
#define ASSERT_TYPE_CMP_wstring_greater(_x1, _x2) (wcscmp(_x1, _x2) > 0)
#define ASSERT_TYPE_CMP_wstring_le(_x1, _x2) (wcscmp(_x1, _x2) <= 0)
#define ASSERT_TYPE_CMP_wstring_ge(_x1, _x2) (wcscmp(_x1, _x2) >= 0)

#define ASSERT_TYPE_CMP_ptr_eq(_x1, _x2) ((_x1) == (_x2))
#define ASSERT_TYPE_CMP_ptr_neq(_x1, _x2) ((_x1) != (_x2))

#define ASSERT_TYPE_CMP_bits_eq(_x1, _x2) ((_x1) == (_x2))
#define ASSERT_TYPE_CMP_bits_neq(_x1, _x2) ((_x1) != (_x2))

#define ASSERT_EQ(_type, _expr1, _expr2) _ASSERT2(_type, _expr1, eq, _expr2)
#define ASSERT_NEQ(_type, _expr1, _expr2) _ASSERT2(_type, _expr1, neq, _expr2)
#define ASSERT_LESS(_type, _expr1, _expr2) _ASSERT2(_type, _expr1, less, _expr2)
#define ASSERT_GREATER(_type, _expr1, _expr2) _ASSERT2(_type, _expr1, greater, _expr2)
#define ASSERT_LE(_type, _expr1, _expr2) _ASSERT2(_type, _expr1, greater, _expr2)
#define ASSERT_GE(_type, _expr1, _expr2) _ASSERT2(_type, _expr1, greater, _expr2)

#define ASSERT_NOT_NULL(_expr) ASSERT_NEQ(ptr, _expr, NULL)
#define ASSERT_NULL(_expr) ASSERT_EQ(ptr, _expr, NULL)

#define ARBITRARY(_type, _min, _max) \
    ((_type)rand() % ((_max) - (_min) + 1) + (_min))
    
#define SET_RANDOM_SEED()                               \
    do {                                                \
        const char *seedstr = getenv("TESTSEED");       \
        unsigned seed;                                  \
                                                        \
        if (seedstr != NULL)                            \
        {                                               \
            seed = (unsigned)strtoul(seedstr, NULL, 0); \
        }                                               \
        else                                            \
        {                                               \
            seed = (unsigned)time(NULL);                \
        }                                               \
        fprintf(stderr, "Random seed is %u\n", seed);   \
        srand(seed);                                    \
    } while(0)

#define TESTSUITE(_description)                 \
    const char testsuite_descr[] = _description

#define TESTCASE(_name, _description, _body)                \
    extern void testcase_##_name(void);                     \
    const char testcase_##_name##_descr[] = _description;   \
    void testcase_##_name(void) \
    {                           \
        _body;                  \
    }                           \
    struct fake

#define TEST_LOG(_fmt, ...)                         \
    fprintf(stderr, "[" _fmt "] ", __VA_ARGS__)

#define TEST_FOREACH(_name, _vars, _body, ...)                          \
    do {                                                                \
        const struct _name##_params {                                   \
            _vars;                                                      \
        } _name##_values[] = {__VA_ARGS__};                             \
        const struct _name##_params *_name = _name##_values;            \
                                                                        \
        for (; _name < _name##_values +                                 \
                 sizeof(_name##_values) / sizeof(*_name##_values);      \
             _name++) {                                                 \
            _body;                                                      \
        }                                                               \
    } while(0)
    


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ASSERTIONS_H */
