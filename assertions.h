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

#include <stddef.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#if !NONFATAL_ASSERTIONS
#define ASSERT_FAIL abort()
#else
static unsigned assert_failure_count;
static bool testcase_failed = false;
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
                    __var2, __var1);                                    \
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

static inline unsigned long long large_rand(unsigned maxbit)
{
    unsigned long long maxval = 1ULL << maxbit;
    unsigned long long result;
    if (maxval < RAND_MAX)
    {
        result = (unsigned long long)rand();
    }
    else
    {
        result = ((unsigned long long)rand() << 32) |
            (unsigned long long)rand();
    }

    return result & (maxval | (maxval - 1));
}

#define ARBITRARY(_type, _min, _max)                    \
    ((_type)large_rand(sizeof(_type) * CHAR_BIT - 1) %  \
     ((_max) - (_min) + 1) + (_min))
    
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

#define TEST_START_MSG(_msg)                    \
    do {                                        \
        fputs(_msg, stderr);                    \
        fputs("...", stderr);                   \
        fflush(stderr);                         \
    } while(0)

#define TEST_OK_MSG  fputs("OK\n", stderr)
#define TEST_FAIL_MSG fputs("FAIL\n", stderr)

#if NONFATAL_ASSERTIONS
#define TEST_START(_msg)                        \
    do {                                        \
        TEST_START_MSG(_msg);                   \
        testcase_failed = false;                \
    } while(0)
#define TEST_END                                \
    do {                                        \
    if (testcase_failed)                        \
        TEST_FAIL_MSG;                          \
    else                                        \
        TEST_OK_MSG;                            \
    } while(0)
#else
#define TEST_START(_msg) TEST_START_MSG(_msg)
#define TEST_END TEST_OK_MSG
#endif

#define TESTVAL_LOG(_id, _type, _val)                               \
    do {                                                            \
        fprintf(stderr, " [" #_id "=" TESTVAL_LOG_FMT_##_type "]",  \
                TESTVAL_LOG_ARGS_##_type(_val));                    \
        fflush(stderr);                                             \
    } while(0)

#if TEST_REPEAT_ARBITRARY > 3
#error "Requested number of test repetitions is too large"
#elif TEST_REPEAT_ARBITRARY == 3
#define TESTVAL_GENERATE_ARBITRARY(_type, _min, _max) \
    ARBITRARY(_type, _min, _max),                     \
        ARBITRARY(_type, _min, _max),                 \
        ARBITRARY(_type, _min, _max)
#elif TEST_REPEAT_ARBITRARY == 2
#define TESTVAL_GENERATE_ARBITRARY(_type, _min, _max) \
    ARBITRARY(_type, _min, _max),                     \
        ARBITRARY(_type, _min, _max)
#else
#define TESTVAL_GENERATE_ARBITRARY(_type, _min, _max)   \
    ARBITRARY(_type, _min, _max)
#endif
    

#define TESTVAL_GENERATE_UINT(_type, _max)                      \
    0, 1, TESTVAL_GENERATE_ARBITRARY(_type, 2, (_max) - 2),     \
        (_max) - 1, (_max)

#define TESTVAL_GENERATE_SINT(_type, _min, _max)                        \
    (_min), (_min) + 1,                                                 \
        TESTVAL_GENERATE_ARBITRARY(_type, (_min) + 2, -2), -1,          \
        TESTVAL_GENERATE_UINT(_type, _max)

#define TESTVAL_GENERATE_BITSET(_type, _max)                    \
    0, TESTVAL_GENERATE_ARBITRARY(_type, 1, (_max) - 1), _max

#define TESTVAL_GENERATE_BITNUM(_type)                                  \
    0,                                                                  \
        TESTVAL_GENERATE_ARBITRARY(unsigned, 1,                         \
                                   (unsigned)sizeof(_type) * CHAR_BIT - 2), \
        (unsigned)sizeof(_type) * CHAR_BIT - 1

#define TESTVAL_GENERATE__bool false, true
#define TESTVAL_LOG_FMT_bool "%s"
#define TESTVAL_LOG_ARGS_bool(_val) ((_val) ? "true" : "false")

typedef unsigned testval_small_uint_t;
typedef int testval_small_int_t;

#define TESTVAL_GENERATE__testval_small_uint_t 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
#define TESTVAL_GENERATE__testval_small_int_t 0, 1, -1, 2, -2, 3, -3, 4, -4
#define TESTVAL_LOG_FMT_testval_small_uint_t "%u"
#define TESTVAL_LOG_FMT_testval_small_int_t "%d"

#define TESTVAL_GENERATE__char                          \
    TEST_GENERATE_ARBITRARY(char, '\x01', '\x1f'),      \
        ' ',                                            \
        TEST_GENERATE_ARBITRARY(char, '!', '/'),        \
        TEST_GENERATE_ARBITRARY(char, '0', '9'),        \
        TEST_GENERATE_ARBITRARY(char, 'A', 'Z'),        \
        '\\',                                           \
        TEST_GENERATE_ARBITRARY(char, 'a', 'z'),        \
        '\x7F'

#define TESTVAL_GENERATE__unsigned_char                         \
    TESTVAL_GENERATE__char,                                     \
        TEST_GENERATE_ARBITRARY(unsigned char, '\x80', '\x9f'), \
        TEST_GENERATE_ARBITRARY(unsigned char, '\xa0', '\xfe'), \
        '\xff'

#define TESTVAL_LOG_FMT_unsigned_char "'\\x%x'"
#define TESTVAL_LOG_FMT_char TESTVAL_LOG_FMT_unsigned_char

#define TESTVAL_GENERATE__uint8_t               \
    TESTVAL_GENERATE_UINT(uint8_t, UINT8_MAX)
#define TESTVAL_GENERATE__int8_t                \
    TESTVAL_GENERATE_SINT(int8_t, INT8_MIN, INT8_MAX)

#define TESTVAL_LOG_FMT_uint8_t PRIu8
#define TESTVAL_LOG_FMT_int8_t  PRId8

#define TESTVAL_GENERATE__wchar_t_bmp                                   \
    TEST_GENERATE_ARBITRARY(wchar_t, L'\x01', L'\x7f'),                 \
        TEST_GENERATE_ARBITRARY(wchar_t, L'\x80', L'\xff'),             \
        TEST_GENERATE_ARBITRARY(wchar_t, L'\x0100', L'\x7ff'),          \
        TEST_GENERATE_ARBITRARY(wchar_t, L'\x0800', L'\xfffe'),         \
        L'\xffff'
        
#if WCHAR_MAX > USHRT_MAX
#define TESTVAL_GENERATE__wchar_t                                    \
    TESTVAL_GENERATE__wchar_t_bmp,                                   \
        TEST_GENERATE_ARBITRARY(wchar_t, L'\x10000',  L'\x1fffff'),  \
        TEST_GENERATE_ARBITRARY(wchar_t, L'\x200000', L'\x3ffffff'),  \
        TEST_GENERATE_ARBITRARY(wchar_t, L'\x4000000', L'\x7ffffff')
#define TESTVAL_LOG_FMT_wchar_t "U+%08x"
#else
#define TESTVAL_GENERATE__wchar_t TESTVAL_GENERATE__wchar_t_bmp
#define TESTVAL_LOG_FMT_wchar_t "U+%04x"
#endif

#define TESTVAL_GENERATE__unsigned_short                \
    TESTVAL_GENERATE_UINT(unsigned short, USHRT_MAX)
#define TESTVAL_GENERATE__short                             \
    TESTVAL_GENERATE_SINT(short, SHRT_MIN, SHRT_MAX)
#define TESTVAL_GENERATE__uint16_t              \
    TESTVAL_GENERATE_UINT(uint16_t, UINT16_MAX)
#define TESTVAL_GENERATE__int16_t                           \
    TESTVAL_GENERATE_SINT(int16_t, INT16_MIN, INT16_MAX)

#define TESTVAL_LOG_FMT_unsigned_short "%u"
#define TESTVAL_LOG_FMT_short "%d"
#define TESTVAL_LOG_FMT_uint16_t "%" PRIu16
#define TESTVAL_LOG_FMT_int16_t "%" PRId16

#define TESTVAL_GENERATE__unsigned              \
    TESTVAL_GENERATE_UINT(unsigned, UINT_MAX)
#define TESTVAL_GENERATE__unsigned_int TESTVAL_GENERATE__unsigned
#define TESTVAL_GENERATE__int                               \
    TESTVAL_GENERATE_SINT(int, INT_MIN, INT_MAX)
#define TESTVAL_GENERATE__uint32_t              \
    TESTVAL_GENERATE_UINT(uint32_t, UINT32_MAX)
#define TESTVAL_GENERATE__int32_t                           \
    TESTVAL_GENERATE_SINT(int32_t, INT32_MIN, INT32_MAX)

#define TESTVAL_LOG_FMT_unsigned "%u"
#define TESTVAL_LOG_FMT_unsigned_int TESTVAL_LOG_FMT_unsigned
#define TESTVAL_LOG_FMT_int "%d"
#define TESTVAL_LOG_FMT_uint32_t "%" PRIu32
#define TESTVAL_LOG_FMT_int32_t "%" PRId32

#define TESTVAL_GENERATE__unsigned_long             \
    TESTVAL_GENERATE_UINT(unsigned long, ULONG_MAX)
#define TESTVAL_GENERATE__long                      \
    TESTVAL_GENERATE_SINT(long, LONG_MIN, LONG_MAX)

#define TESTVAL_LOG_FMT_unsigned_long "%lu"
#define TESTVAL_LOG_FMT_long "%ld"

#define TESTVAL_GENERATE__unsigned_long_long    \
    TESTVAL_GENERATE_UINT(unsigned long long, ULLONG_MAX)
#define TESTVAL_GENERATE__long_long                 \
    TESTVAL_GENERATE_SINT(long, LONG_MIN, LONG_MAX)
#define TESTVAL_GENERATE__uint64_t              \
    TESTVAL_GENERATE_UINT(uint64_t, UINT64_MAX)
#define TESTVAL_GENERATE__int64_t                           \
    TESTVAL_GENERATE_SINT(int32_t, INT64_MIN, INT64_MAX)

#define TESTVAL_LOG_FMT_unsigned_long_long "%llu"
#define TESTVAL_LOG_FMT_long_long "%lld"
#define TESTVAL_LOG_FMT_uint64_t "%" PRIu64
#define TESTVAL_LOG_FMT_int64_t "%" PRId64

#define TESTVAL_GENERATE__size_t                \
    TESTVAL_GENERATE_UINT(size_t, SIZE_MAX)              
#define TESTVAL_LOG_FMT_size_t "%zu"
    
#define TESTVAL_GENERATE__uintptr_t                 \
    TESTVAL_GENERATE_BITSET(uintptr_t, UINTPTR_MAX)
#define TESTVAL_LOG_FMT_uintptr_t "%016" PRIxPTR

typedef uint8_t testval_bitset8_t;
typedef uint16_t testval_bitset16_t;
typedef uint32_t testval_bitset32_t;
typedef uint64_t testval_bitset64_t;

#define TESTVAL_GENERATE__testval_bitset8_t                 \
    TESTVAL_GENERATE_BITSET(testval_bitset8_t, UINT8_MAX)
#define TESTVAL_GENERATE__testval_bitset16_t                \
    TESTVAL_GENERATE_BITSET(testval_bitset16_t, UINT16_MAX)
#define TESTVAL_GENERATE__testval_bitset32_t                \
    TESTVAL_GENERATE_BITSET(testval_bitset32_t, UINT32_MAX)
#define TESTVAL_GENERATE__testval_bitset64_t                \
    TESTVAL_GENERATE_BITSET(testval_bitset64_t, UINT64_MAX)

#define TESTVAL_LOG_FMT_testval_bitset8_t "%02x"
#define TESTVAL_LOG_FMT_testval_bitset16_t "%04x"
#define TESTVAL_LOG_FMT_testval_bitset32_t "%08x"
#define TESTVAL_LOG_FMT_testval_bitset64_t "%016x"

typedef unsigned testval_bitnum8_t;
typedef unsigned testval_bitnum16_t;
typedef unsigned testval_bitnum32_t;
typedef unsigned testval_bitnum64_t;
typedef unsigned testval_bitnum_short_t;
typedef unsigned testval_bitnum_int_t;
typedef unsigned testval_bitnum_long_t;
typedef unsigned testval_bitnum_size_t;
typedef unsigned testval_bitnum_ptr_t;

#define TESTVAL_GENERATE__testval_bitnum8_t TESTVAL_GENERATE_BITNUM(uint8_t)
#define TESTVAL_GENERATE__testval_bitnum16_t TESTVAL_GENERATE_BITNUM(uint16_t)
#define TESTVAL_GENERATE__testval_bitnum32_t TESTVAL_GENERATE_BITNUM(uint32_t)
#define TESTVAL_GENERATE__testval_bitnum64_t TESTVAL_GENERATE_BITNUM(uint64_t)
#define TESTVAL_GENERATE__testval_bitnum_short_t TESTVAL_GENERATE_BITNUM(unsigned short)
#define TESTVAL_GENERATE__testval_bitnum_int_t TESTVAL_GENERATE_BITNUM(unsigned)
#define TESTVAL_GENERATE__testval_bitnum_long_t TESTVAL_GENERATE_BITNUM(unsigned long)
#define TESTVAL_GENERATE__testval_bitnum_size_t TESTVAL_GENERATE_BITNUM(size_t)
#define TESTVAL_GENERATE__testval_bitnum_ptr_t TESTVAL_GENERATE_BITNUM(void *)

#define TESTVAL_LOG_FMT_testval_bitnum8_t "%u"
#define TESTVAL_LOG_FMT_testval_bitnum16_t "%u"
#define TESTVAL_LOG_FMT_testval_bitnum32_t "%u"
#define TESTVAL_LOG_FMT_testval_bitnum64_t "%u"
#define TESTVAL_LOG_FMT_testval_bitnum_short_t "%u"
#define TESTVAL_LOG_FMT_testval_bitnum_int_t "%u"
#define TESTVAL_LOG_FMT_testval_bitnum_long_t "%u"
#define TESTVAL_LOG_FMT_testval_bitnum_size_t "%u"
#define TESTVAL_LOG_FMT_testval_bitnum_ptr_t "%u"

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ASSERTIONS_H */
