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

#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#define ASSERT(_cond) assert(_cond)
#define ASSERT_NOT(_cond) assert(!(_cond))

#define __ASSERT_CMP_INLINE(_name, _type, _fmt, _cmp)                   \
    static inline void assert_##_name(const char *_descr,               \
                                      const char *_file,                \
                                      const char *_func,                \
                                      unsigned _line,                   \
                                      _type _arg1, _type _arg2)         \
    {                                                                   \
        if (!(_cmp))                                                    \
        {                                                               \
            fprintf(stderr, "Assertion %s failed at %s(%s):%u: "        \
                    _fmt " vs. " _fmt "\n",                             \
                    _descr, _file, _func, _line, _arg1, _arg2);         \
            abort();                                                    \
        }                                                               \
    }                                                                   \
    struct fake

#define __ASSERT_CMP_INLINE_PACK(_name, _type, _fmt,                    \
                                 _cmpeq, _cmpne, _cmpless, _cmpgtr,     \
                                 _cmple, _cmpge)                        \
    __ASSERT_CMP_INLINE(_name##_eq, _type, _fmt, _cmpeq);               \
    __ASSERT_CMP_INLINE(_name##_neq, _type, _fmt, _cmpne);              \
    __ASSERT_CMP_INLINE(_name##_less, _type, _fmt, _cmpless);           \
    __ASSERT_CMP_INLINE(_name##_greater, _type, _fmt, _cmpgtr);         \
    __ASSERT_CMP_INLINE(_name##_le, _type, _fmt, _cmple);               \
    __ASSERT_CMP_INLINE(_name##_ge, _type, _fmt, _cmpge)               \

#define __ASSERT_CMP_WRAP(_name, _cmpdescr, _arg1, _arg2)               \
    assert_##_name(#_arg1 " " _cmpdescr " " #_arg2, __FILE__,                   \
                   __FUNCTION__, __LINE__,                              \
                   (_arg1), (_arg2))

__ASSERT_CMP_INLINE_PACK(int, long, "%ld",
                         _arg1 == _arg2, _arg1 != _arg2,
                         _arg1 < _arg2, _arg1 > _arg2,
                         _arg1 <= _arg2, _arg1 >= _arg2);
__ASSERT_CMP_INLINE_PACK(uint, unsigned long, "%lu",
                         _arg1 == _arg2, _arg1 != _arg2,
                         _arg1 < _arg2, _arg1 > _arg2,
                         _arg1 <= _arg2, _arg1 >= _arg2);
__ASSERT_CMP_INLINE_PACK(float, double, "%g",
                         _arg1 == _arg2, _arg1 != _arg2,
                         _arg1 < _arg2, _arg1 > _arg2,
                         _arg1 <= _arg2, _arg1 >= _arg2);
__ASSERT_CMP_INLINE_PACK(str, const char *, "%s",
                         strcmp(_arg1, _arg2) == 0,
                         strcmp(_arg1, _arg2) != 0,
                         strcmp(_arg1, _arg2) < 0,
                         strcmp(_arg1, _arg2) > 0,
                         strcmp(_arg1, _arg2) <= 0,
                         strcmp(_arg1, _arg2) >= 0);
__ASSERT_CMP_INLINE_PACK(wstr, const wchar_t *, "%ls",
                         wcscmp(_arg1, _arg2) == 0,
                         wcscmp(_arg1, _arg2) != 0,
                         wcscmp(_arg1, _arg2) < 0,
                         wcscmp(_arg1, _arg2) > 0,
                         wcscmp(_arg1, _arg2) <= 0,
                         wcscmp(_arg1, _arg2) >= 0);
__ASSERT_CMP_INLINE(ptr_eq, const void *, "%p", _arg1 == _arg2);
__ASSERT_CMP_INLINE(ptr_neq, const void *, "%p", _arg1 != _arg2);
__ASSERT_CMP_INLINE(bits_eq, unsigned long long, "%llx", _arg1 == _arg2);
__ASSERT_CMP_INLINE(bits_neq, unsigned long long, "%llx", _arg1 != _arg2);

#define ASSERT_INT_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(int_eq, "==", _arg1, _arg2)
#define ASSERT_INT_NEQ(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(int_neq, "!=", _arg1, _arg2)
#define ASSERT_INT_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(int_eq, "==", _arg1, _arg2)
#define ASSERT_INT_LESS(_arg1, _arg2)               \
    __ASSERT_CMP_WRAP(int_less, "<", _arg1, _arg2)
#define ASSERT_INT_GREATER(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(int_greater, ">", _arg1, _arg2)
#define ASSERT_INT_LE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(int_le, "<=", _arg1, _arg2)
#define ASSERT_INT_GE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(int_ge, ">=", _arg1, _arg2)

#define ASSERT_UINT_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(uint_eq, "==", _arg1, _arg2)
#define ASSERT_UINT_NEQ(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(uint_neq, "!=", _arg1, _arg2)
#define ASSERT_UINT_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(uint_eq, "==", _arg1, _arg2)
#define ASSERT_UINT_LESS(_arg1, _arg2)               \
    __ASSERT_CMP_WRAP(uint_less, "<", _arg1, _arg2)
#define ASSERT_UINT_GREATER(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(uint_greater, ">", _arg1, _arg2)
#define ASSERT_UINT_LE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(uint_le, "<=", _arg1, _arg2)
#define ASSERT_UINT_GE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(uint_ge, ">=", _arg1, _arg2)

#define ASSERT_FLOAT_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(float_eq, "==", _arg1, _arg2)
#define ASSERT_FLOAT_NEQ(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(float_neq, "!=", _arg1, _arg2)
#define ASSERT_FLOAT_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(float_eq, "==", _arg1, _arg2)
#define ASSERT_FLOAT_LESS(_arg1, _arg2)               \
    __ASSERT_CMP_WRAP(float_less, "<", _arg1, _arg2)
#define ASSERT_FLOAT_GREATER(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(float_greater, ">", _arg1, _arg2)
#define ASSERT_FLOAT_LE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(float_le, "<=", _arg1, _arg2)
#define ASSERT_FLOAT_GE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(float_ge, ">=", _arg1, _arg2)

#define ASSERT_STR_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(str_eq, "==", _arg1, _arg2)
#define ASSERT_STR_NEQ(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(str_neq, "!=", _arg1, _arg2)
#define ASSERT_STR_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(str_eq, "==", _arg1, _arg2)
#define ASSERT_STR_LESS(_arg1, _arg2)               \
    __ASSERT_CMP_WRAP(str_less, "<", _arg1, _arg2)
#define ASSERT_STR_GREATER(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(str_greater, ">", _arg1, _arg2)
#define ASSERT_STR_LE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(str_le, "<=", _arg1, _arg2)
#define ASSERT_STR_GE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(str_ge, ">=", _arg1, _arg2)

#define ASSERT_WSTR_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(wstr_eq, "==", _arg1, _arg2)
#define ASSERT_WSTR_NEQ(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(wstr_neq, "!=", _arg1, _arg2)
#define ASSERT_WSTR_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(wstr_eq, "==", _arg1, _arg2)
#define ASSERT_WSTR_LESS(_arg1, _arg2)               \
    __ASSERT_CMP_WRAP(wstr_less, "<", _arg1, _arg2)
#define ASSERT_WSTR_GREATER(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(wstr_greater, ">", _arg1, _arg2)
#define ASSERT_WSTR_LE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(wstr_le, "<=", _arg1, _arg2)
#define ASSERT_WSTR_GE(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(wstr_ge, ">=", _arg1, _arg2)

#define ASSERT_PTR_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(ptr_eq, "==", _arg1, _arg2)
#define ASSERT_PTR_NEQ(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(ptr_neq, "!=", _arg1, _arg2)

#define ASSERT_BITS_EQ(_arg1, _arg2)                 \
    __ASSERT_CMP_WRAP(bits_eq, "==", _arg1, _arg2)
#define ASSERT_BITS_NEQ(_arg1, _arg2)                \
    __ASSERT_CMP_WRAP(bits_neq, "!=", _arg1, _arg2)


#define ARBITRARY(_type, _min, _max) \
    (_type)(rand() % ((_max) - (_min) + 1) + (_min))
    
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

#define BEGIN_TESTSUITE(_msg) fprintf(stderr, "%s:\n", (_msg))
#define BEGIN_TESTCASE(_msg) BEGIN_TESTSUITE(_msg)

#define BEGIN_TESTSTEP_VERBOSE(_indent, _msg)            \
    do {                                                \
        fprintf(stderr, "%*s", 2 * (_indent), (_msg));   \
        fflush(stderr);                                 \
    } while(0)
#define BEGIN_TESTSTEP_SILENT(_indent, _msg) ((void)0)

#define BEGIN_TESTSTEP_CLEANUP BEGIN_TESTSTEP_SILENT
#define BEGIN_TESTSTEP_PREREQ BEGIN_TESTSTEP_VERBOSE
#define BEGIN_TESTSTEP_CONDITION BEGIN_TESTSTEP_VERBOSE
#define BEGIN_TESTSTEP_ASSERTION BEGIN_TESTSTEP_VERBOSE
#define BEGIN_TESTSTEP_DESCRIPTION BEGIN_TESTSTEP_VERBOSE

#define TESTITER_level_chars ".o*+#"

#define BEGIN_TESTITER_VERBOSE(_level)                                  \
    do {                                                                \
        assert((_level) > 0 && (_level) < sizeof(TESTITER_level_chars)); \
        fputc(TESTITER_level_chars[(_level) - 1], stderr);              \
        fflush(stderr);                                                 \
    } while(0)

#define BEGIN_TESTITER_SILENT(_level) ((void)0)

#define BEGIN_TESTITER_CLEANUP BEGIN_TESTITER_SILENT
#define BEGIN_TESTITER_PREREQ BEGIN_TESTITER_SILENT
#define BEGIN_TESTITER_CONDITION BEGIN_TESTITER_SILENT
#define BEGIN_TESTITER_ASSERTION BEGIN_TESTITER_VERBOSE
#define BEGIN_TESTITER_DESCRIPTION BEGIN_TESTITER_SILENT

#define BEGIN_TESTSUBSTEP_VERBOSE() fputc('\n', stderr)
#define BEGIN_TESTSUBSTEP_SILENT() ((void)0)

#define BEGIN_TESTSUBSTEP_CLEANUP BEGIN_TESTSUBSTEP_SILENT
#define BEGIN_TESTSUBSTEP_PREREQ BEGIN_TESTSUBSTEP_VERBOSE
#define BEGIN_TESTSUBSTEP_CONDITION BEGIN_TESTSUBSTEP_VERBOSE
#define BEGIN_TESTSUBSTEP_ASSERTION BEGIN_TESTSUBSTEP_VERBOSE
#define BEGIN_TESTSUBSTEP_DESCRIPTION BEGIN_TESTSUBSTEP_VERBOSE

#define END_TESTSTEP_VERBOSE(_indent)               \
    fprintf(stderr, "%*s\n", 2 * (_indent), " OK")
#define END_TESTSTEP_NEWLINE(_indent) fputc('\n', stderr)
#define END_TESTSTEP_SILENT(_indent) ((void)0)

#define END_TESTSTEP_CLEANUP END_TESTSTEP_SILENT
#define END_TESTSTEP_PREREQ END_TESTSTEP_NEWLINE
#define END_TESTSTEP_CONDITION END_TESTSTEP_NEWLINE
#define END_TESTSTEP_ASSERTION END_TESTSTEP_VERBOSE
#define END_TESTSTEP_DESCRIPTION END_TESTSTEP_NEWLINE


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ASSERTIONS_H */
