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

#include <ctype.h>
#include <uniname.h>
#include <apr_date.h>
#include "engine.h"


expr_value parse_iso_timestamp(exec_context *ctx, const char *ts)
{
    struct tm parsed;
    int rc;

    memset(&parsed, 0, sizeof(parsed));
    rc = sscanf(ts, "%d-%d-%dT%d:%d:%d", 
                &parsed.tm_year, &parsed.tm_mon, &parsed.tm_mday,
                &parsed.tm_hour, &parsed.tm_min, &parsed.tm_sec);
    if (rc == EOF)
        raise_error(ctx, apr_get_netos_error());
    if (rc != 3 && rc != 6)
        raise_error(ctx, APR_EBADDATE);
    parsed.tm_mon--;
    parsed.tm_year -= 1900;
    
    return MAKE_VALUE(TIMESTAMP, ctx->static_pool, mktime(&parsed));
}

expr_value parse_net_timestamp(exec_context *ctx, const char *ts)
{
    apr_time_t parsed;

    parsed = apr_date_parse_rfc(ts);
    if (parsed == APR_DATE_BAD)
        raise_error(ctx, APR_EBADDATE);
    return MAKE_NUM_VALUE((double)apr_time_sec(parsed));
}

time_t timestamp_just_date(time_t ts)
{
    struct tm t = *localtime(&ts);
    t.tm_sec = t.tm_min = t.tm_hour = 0;
    return mktime(&t);
}

static inline uint8_t hex_digit_value(int ch)
{
    if (isdigit(ch))
        return (uint8_t)(ch - '0');
    else
        return (uint8_t)(tolower(ch) - 'a' + 10);
}

expr_value decode_hex(exec_context *ctx, const char *hex)
{
    size_t len = strlen(hex);
    uint8_t *data, *ptr;
    bool even = true;

    len = (len + 1) / 2;
    data = apr_palloc(ctx->local_pool, len);

    for (ptr = data; *hex != '\0'; hex++, even = !even)
    {
        uint8_t val;
        if (!isxdigit(*hex))
            raise_error(ctx, APR_FROM_OS_ERROR(EINVAL));
        val = hex_digit_value(*hex);
        if (even)
            *ptr = val;
        else
        {
            *ptr = (uint8_t)(*ptr << 4) | val;
            ptr++;
        }
    }
    return MAKE_VALUE(BINARY, ctx->local_pool, 
                      ((binary_data){.type = &default_binary_data,
                              .len = len,
                              .data = data}));
            
}

double parse_duration(exec_context *ctx, const char *dur)
{
    struct tm collect;

    memset(&collect, 0, sizeof(collect));
    
    while (*dur != '\0' && *dur != 'T')
    {
        unsigned long val;
        
        if (!isdigit(*dur))
            raise_error(ctx, APR_FROM_OS_ERROR(EINVAL));
        val = strtoul(dur, (char **)&dur, 10);
        switch (*dur) 
        {
            case '\0':
            case 'T':
                /* do nothing */
                break;
            case 'Y':
                collect.tm_year = (int)val;
                break;
            case 'M':
                collect.tm_mon = (int)val;
                break;
            case 'D':
                collect.tm_mday = (int)val + 1;
                break;
            case 'W':
                collect.tm_mday = (int)(7 * val) + 1;
                break;
            default:
                raise_error(ctx, APR_EINVAL);
        }
    }
    if (*dur != '\0')
    {
        dur++;
        while (*dur != '\0')
        {
            unsigned val;
            
            if (!isdigit(*dur))
                raise_error(ctx, APR_EINVAL);
            val = (unsigned)strtoul(dur, (char **)&dur, 10);
            switch (*dur) 
            {
                case '\0':
                    /* do nothing */
                    break;
                case 'H':
                    collect.tm_hour = (int)val;
                    break;
                case 'M':
                    collect.tm_min = (int)val;
                    break;
                case 'S':
                    collect.tm_sec = (int)val;
                    break;
            }
        }
    }
    return (double)mktime(&collect);
}

static void decode_unicode_char(exec_context *ctx, const char **str, 
                                uint8_t **dest, size_t size)
{
    ucs4_t uch = 0;
    int enclen;
    size_t limit = 0;

    if (**str == '+')
    {
        for ((*str)++; limit < size; limit++, (*str)++)
        {
            if (!isxdigit(**str))
                break;
            uch = (uch << 4) | hex_digit_value(**str);
        }
        limit++;
        (*str)--;
    }
    else if (**str == '{')
    {
        const char *found = strchr(*str + 1, '}');
        if (found == NULL)
            raise_error(ctx, APR_EINVAL);
        else
        {
            char name[found - *str];

            memcpy(name, *str + 1, (size_t)(found - *str - 1));
            name[found - *str - 1] = '\0';
            uch = unicode_name_character(name);
            if (uch == UNINAME_INVALID)
                raise_error(ctx, APR_ENOENT);
            *str = found;
            limit = (size_t)(found - *str + 1);
        }
    }
    else
        raise_error(ctx, APR_EINVAL);

    enclen = u8_uctomb(*dest, uch, (int)(2 + limit));
    if (enclen == -1)
        raise_error(ctx, APR_EINVAL);
    if (enclen == -2)
        raise_error(ctx, APR_ENOSPC);
    (*dest) += enclen;
}

expr_value unquote_string(exec_context *ctx, const char *str)
{
    uint8_t *expanded = apr_palloc(ctx->local_pool, strlen(str) + 1);
    uint8_t *dest;
    
    for (dest = expanded; *str != '\0'; str++)
    {
        if (*str != '\\')
            *dest++ = (uint8_t)*str;
        else
        {
            str++;
            if (*str == '\0')
                raise_error(ctx, APR_EINVAL);
            if (*str == 'u')
            {
                str++;
                decode_unicode_char(ctx, &str, &dest, 4);
            }
            else if (*str == 'U')
            {
                str++;
                decode_unicode_char(ctx, &str, &dest, 8);
            }
            else
            {
                static const char esc[] = "abfnrtv";
                static const char codes[] = "\a\b\f\n\r\t\v";
                const char *found = strchr(esc, *str);
                
                *dest++ = (uint8_t)(found ? codes[found - esc] : *str);
            }
        }
    }

    return MAKE_VALUE(STRING, ctx->local_pool, expanded);
}

const uint8_t *find_binary(binary_data data, binary_data search)
{
    uint8_t *next;
    
    if (search.len == 0)
        return data.data;

    for (;;)
    {
        next = memchr(data.data, search.data[0], data.len);
        if (next == NULL)
            break;
        if (next + search.len > data.data + data.len)
        {
            next = NULL;
            break;
        }
        if (memcmp(next, search.data, search.len) == 0)
            break;
        data.len -= (size_t)(next - data.data) + 1u;
        data.data = next + 1;
    } 

    return next;
}

size_t normalize_index(exec_context *ctx, size_t len, double pos)
{
    if (isinf(pos))
    {
        if (len == 0)
            raise_error(ctx, TEN_OUT_OF_RANGE);
        return len - 1;
    }
    if (pos < 0.0)
    {
        pos += (double)len;
    }
    if (pos < 0.0 || pos >= (size_t)len)
        raise_error(ctx, TEN_OUT_OF_RANGE);
    if (pos < 1.0)
        pos *= (double)len;
    return (size_t)pos;
}

expr_value extract_substr(exec_context *ctx, const uint8_t *str, 
                          size_t start, size_t len,
                          const uint8_t **end)
{
    expr_value result;
    size_t count;
    int ucslen;
    
    for (count = 0; count < start && *str; str += ucslen, count++)
    {
        ucslen = u8_strmblen(str);
        if (ucslen < 0)
            raise_error(ctx, APR_EINVAL);
    }
    if (!len || !*str)
        result = MAKE_VALUE(STRING, ctx->static_pool, (const uint8_t *)"");
    else
    {
        const uint8_t *start = str;
        
        for (count = 0; count < len && *str; str += ucslen, count++)
        {
            ucslen = u8_strmblen(str);
            if (ucslen < 0)
                raise_error(ctx, APR_EINVAL);
        }
        result = MAKE_VALUE(STRING, ctx->local_pool,
                            (const uint8_t *)
                            apr_pstrndup(ctx->local_pool,
                                         (const char *)start,
                                         (apr_size_t)(str - start)));
    }
    if (end != NULL)
        *end = str;
    return result;
}

expr_value generic_split(exec_context *ctx, void *data,
                         bool (*isend)(exec_context *, const void *),
                         expr_value (*getprefix)(exec_context *ctx, void *, size_t),
                         size_t chunksize)
{
    expr_value result = make_expr_value_list(ctx, 0);
    
    while (!isend(ctx, data))
    {
        add_expr_list_value(ctx, &result,
                            getprefix(ctx, data, chunksize));
    }
    return result;
}

expr_value generic_tokenize(exec_context *ctx, void *data, const void *sep,
                            size_t (*find)(exec_context *, const void *, const void *),
                            void (*skip)(exec_context *, void *, const void *),
                            expr_value (*getprefix)(exec_context *ctx, void *, size_t))
{
    expr_value result = make_expr_value_list(ctx, 0);
    size_t len;
    
    for (;;)
    {
        if (skip)
            skip(ctx, data, sep);
        len = find(ctx, data, sep);
        if (len == (size_t)(-1))
            break;
        add_expr_list_value(ctx, &result,
                            getprefix(ctx, data, len));
    }
    return result;
}

expr_value generic_range(exec_context *ctx, const void *data, 
                         double start, double end,
                         size_t (*getlen)(exec_context *, const void *),
                         expr_value (*extract)(exec_context *, const void *, size_t, size_t))
{
    size_t len = getlen(ctx, data);
    size_t istart = normalize_index(ctx, len, start);
    size_t iend = normalize_index(ctx, len, end);
    
    return extract(ctx, data, istart, istart > iend ? 0u : iend - istart + 1);
}
