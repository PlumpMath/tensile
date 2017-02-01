/**********************************************************************
 * Copyright (c) 2017 Artem V. Andreev
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**********************************************************************/

/** @file
 * @author Artem V. Andreev <artem@iling.spb.ru>
 */

#if DO_TESTS
#include <stdio.h>
#endif
#include "dstring.h"

tn_string
tn_strdup(const char *str)
{
    if (str == NULL || *str == '\0')
        return TN_EMPTY_STRING;
    else
    {
        size_t len = strlen(str);
        char *copy = tn_alloc_blob(len + 1);

        memcpy(copy, str, len + 1);

        return (tn_string){.len = len, .str = copy};
    }
}

#if DO_TESTS
static void test_strdup(void)
{
    const char src[] = "abcdefghijk\0mno";
    tn_string s = tn_strdup(src);

    assert(s.str != src);
    assert(s.len == sizeof(src) - 5);
    assert(memcmp(s.str, src, sizeof(src) - 4) == 0);

    assert(tn_strdup("").str == NULL);
}
#endif

tn_string
tn_strdupmem(size_t len, const uint8_t *data)
{
    if (data == NULL || len == 0)
    {
        assert(len == 0);
        return TN_EMPTY_STRING;
    }
    else
    {
        char *copy = tn_alloc_blob(len + 1);

        memcpy(copy, data, len);
        copy[len] = '\0';

        return (tn_string){.len = len, .str = copy};
    }
}

#if DO_TESTS
static void test_strdupmem(void)
{
    const char src[] = "abcd\0\0\0efghijk";
    tn_string s = tn_strdupmem(sizeof(src) - 1, (const uint8_t *)src);

    assert(s.str != src);
    assert(s.len == sizeof(src) - 1);
    assert(memcmp(s.str, src, sizeof(src)) == 0);

    assert(tn_strdupmem(0, NULL).str == NULL);
}
#endif


const char *
tn_str2cstr(tn_string str)
{
    if (str.str == NULL)
        return "";
    /* This is safe, because any data in tn_string
     * is either NUL-terminated or a substring of another tn_string
     */
    if (str.str[str.len] == '\0')
        return str.str;
    else
    {
        char *buf = tn_alloc_blob(str.len + 1);

        memcpy(buf, str.str, str.len);
        buf[str.len] = '\0';
        return buf;
    }
}

#if DO_TESTS
static void test_str2cstr(void)
{
    static const char literal[] = "abcdefghi";
    tn_string copy;
    const char *chunk;
    
    assert(*tn_str2cstr(TN_EMPTY_STRING) == '\0');
    assert(tn_str2cstr(TN_STRING_LITERAL(literal)) == literal);
    assert(tn_str2cstr(tn_cstr2str(literal)) == literal);
    
    copy = tn_strdup(literal);
    assert(tn_str2cstr(copy) == copy.str);
    
    chunk = tn_str2cstr(tn_substr(copy, 0, 1));
    assert(chunk != copy.str);
    assert(chunk[0] == copy.str[0]);
    assert(chunk[1] == '\0');

}
#endif

int
tn_strcmp(tn_string str1, tn_string str2)
{
    size_t minlen = str1.len < str2.len ? str1.len : str2.len;
    int result = memcmp(str1.str, str2.str, minlen);

    if (!result)
        result = str1.len < str2.len ? -1 : str1.len > str2.len ? 1 : 0;

    return result;
}

#if DO_TESTS
static void test_strcmp(void)
{
    tn_string test1 = TN_STRING_LITERAL("abc");
    tn_string test2 = TN_STRING_LITERAL("def");
    tn_string test3 = TN_STRING_LITERAL("abcdef");
    tn_string test3z = TN_STRING_LITERAL("abc\0\0def");
    tn_string testz = TN_STRING_LITERAL("\0");
    
    assert(tn_strcmp(TN_EMPTY_STRING, TN_EMPTY_STRING) == 0);
    assert(tn_strcmp(TN_EMPTY_STRING, test1) < 0);
    assert(tn_strcmp(test1, TN_EMPTY_STRING) > 0);    
    assert(tn_strcmp(test1, test1) == 0);
    assert(tn_strcmp(test1, test2) < 0);
    assert(tn_strcmp(test2, test1) > 0);
    assert(tn_strcmp(test1, test3) < 0);
    assert(tn_strcmp(test3, test1) > 0);
    assert(tn_strcmp(test3z, test1) > 0);
    assert(tn_strcmp(testz, TN_EMPTY_STRING) > 0);
}
#endif

tn_string
tn_strcat(tn_string str1, tn_string str2)
{
    char *buf;
    
    if (str2.len == 0)
        return str1;
    if (str1.len == 0)
        return str2;

    buf = tn_alloc_blob(str1.len + str2.len + 1);
    memcpy(buf, str1.str, str1.len);
    memcpy(buf + str1.len, str2.str, str2.len);
    buf[str1.len + str2.len] = '\0';

    return (tn_string){.str = buf, .len = str1.len + str2.len};
}

#if DO_TESTS
static void test_strcat(void)
{
    tn_string str1 = TN_STRING_LITERAL("abcd\0efghi");
    tn_string str2 = TN_STRING_LITERAL("ijkmn\0opqr");
    tn_string str3 = TN_STRING_LITERAL("abcd\0efghiijkmn\0opqr");

    assert(tn_strcmp(tn_strcat(str1, str2), str3) == 0);
    assert(tn_strcat(str1, TN_EMPTY_STRING).str == str1.str);
    assert(tn_strcat(TN_EMPTY_STRING, str2).str == str2.str);
}
#endif

tn_string
tn_straddch(tn_string str, char ch)
{
    char *buf = tn_alloc_blob(str.len + 2);

    memcpy(buf, str.str, str.len);
    buf[str.len] = ch;
    buf[str.len + 1] = '\0';

    return (tn_string){.str = buf, .len = str.len + 1};
}

#if DO_TESTS
static void test_straddch(void)
{
    tn_string str = TN_STRING_LITERAL("abcd\0efghi");
    tn_string str2 = TN_STRING_LITERAL("abcd\0efghiz");
    tn_string str3 = TN_STRING_LITERAL("abcd\0efghi\0");
    tn_string str4 = TN_STRING_LITERAL("z");
    tn_string str5 = TN_STRING_LITERAL("\0");

    assert(tn_strcmp(tn_straddch(str, 'z'), str2) == 0);
    assert(tn_strcmp(tn_straddch(str, '\0'), str3) == 0);
    assert(tn_strcmp(tn_straddch(TN_EMPTY_STRING, 'z'), str4) == 0);
    assert(tn_strcmp(tn_straddch(TN_EMPTY_STRING, '\0'), str5) == 0);
}
#endif

tn_string
tn_strcats(size_t n, tn_string strs[var_size(n)], tn_string sep)
{
    if (n == 0)
        return TN_EMPTY_STRING;
    else if (n == 1)
        return strs[0];
    else
    {
        size_t i;
        size_t len = strs[0].len;
        char *buf;
        char *ptr;

        for (i = 1; i < n; i++)
        {
            len += sep.len;
            len += strs[i].len;
        }

        buf = tn_alloc_blob(len + 1);
        memcpy(buf, strs[0].str, strs[0].len);
        for (ptr = buf + strs[0].len, i = 1; i < n; i++)
        {
            memcpy(ptr, sep.str, sep.len);
            ptr += sep.len;
            memcpy(ptr, strs[i].str, strs[i].len);
        }

        return (tn_string){.str = buf, .len = len};
    }
}

#if DO_TESTS
static void test_strcats(void)
{
    tn_string strs[] = {
        TN_STRING_LITERAL("abc"),
        TN_STRING_LITERAL("def"),
        TN_EMPTY_STRING,
        TN_STRING_LITERAL("gh\0i"),
        TN_STRING_LITERAL("jkl"),
    };
    tn_string sep = TN_STRING_LITERAL(",");
    tn_string sep1 = TN_STRING_LITERAL("\0");
    tn_string result1 = TN_STRING_LITERAL("abcdefgh\0ijkl");
    tn_string result2 = TN_STRING_LITERAL("abc,def,,gh\0i,jkl");
    tn_string result3 = TN_STRING_LITERAL("abc\0def,,gh\0i,jkl");
}
#endif

tn_string
tn_substr(tn_string str, size_t pos, size_t len)
{
    if (pos >= str.len)
        return TN_EMPTY_STRING;
    if (pos + len > str.len)
        len = str.len - pos;
    if (len == 0)
        return TN_EMPTY_STRING;

    return (tn_string){.str = str.str + pos, .len = len};
}

tn_string
tn_strcut(tn_string str, size_t pos, size_t len)
{
    if (pos >= str.len || len == 0)
        return str;

    if (pos + len >= str.len)
        return (tn_string){.str = str.str, .len = pos};
    else
    {
        char *buf = tn_alloc_blob(str.len - len + 1);

        memcpy(buf, str.str, pos);
        memcpy(buf + pos, str.str + pos + len, str.len - pos - len);

        return (tn_string){.str = buf, .len = str.len - len};
    }
}

bool
tn_strrchr(tn_string str, char ch, size_t *pos)
{
    size_t i;

    for (i = str.len; i > 0; i--)
    {
        if (str.str[i - 1] == ch)
        {
            if (pos)
                *pos = i;
            return true;
        }
    }
  
    return false;
}

bool
tn_strstr(tn_string str, tn_string sub, size_t *pos)
{
    size_t i;

    if (sub.len == 0)
    {
        if (pos)
            *pos = 0;
        return true;
    }
    
    for (i = 0; i < str.len - sub.len + 1; i++)
    {
        size_t j;

        for (j = 0; j < sub.len; j++)
        {
            if (str.str[i + j] != sub.str[j])
                break;
        }
        if (j == sub.len)
        {
            if (pos)
                *pos = i;
            return true;
        }
    }
    return false;
}

bool
tn_strtok(tn_string *src, const char *sep, tn_string *tok)
{
    size_t i;
    size_t start;

    for (i = 0; i < src->len; i++)
    {
        if (strchr(sep, src->str[i]) == NULL)
            break;
    }
    if (i == src->len)
        return false;

    start = i;
    for (; i < src->len; i++)
    {
        if (strchr(sep, src->str[i]) != NULL)
            break;
    }
    if (tok)
    {
        tok->str = src->str + start;
        tok->len = i - start;
    }
    src->str += i;
    src->len -= i;

    return true;
}

tn_string tn_strmap(tn_string str, char (*func)(char ch))
{
    size_t i;
    char *buf;
    
    if (str.len == 0)
        return TN_EMPTY_STRING;

    buf = tn_alloc_blob(str.len + 1);
    for (i = 0; i < str.len; i++)
        buf[i] = func(str.str[i]);

    return (tn_string){.str = buf, .len = str.len};
}

#if DO_TESTS
int main()
{
    GC_INIT();
    test_strdup();
    test_strdupmem();
    test_strcmp();
    test_str2cstr();
    test_strcat();
    test_straddch();
    puts("OK");
    
    return 0;
}
#endif
