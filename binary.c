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

#include "engine.h"


static const uint8_t *default_binary_tostring(exec_context *ctx, binary_data data)
{
    uint8_t *result;
    if (!u8_check(data.data, data.len))
        raise_error(ctx, APR_EINVAL);
    
    result = apr_palloc(ctx->local_pool, data.len + 1);
    memcpy(result, data.data, data.len);
    result[data.len] = '\0';
    return result;
}

static ATTR_PURE
double default_binary_tonumber(exec_context *ctx ATTR_UNUSED, binary_data data)
{
    unsigned long long result = 0;
    unsigned i;
    
    for (i = 0; i < data.len; i++)
    {
        result = (result << 8) | data.data[i];
    }
    return (double)result;
}

static expr_value default_binary_reverse(exec_context *ctx, binary_data data)
{
    binary_data result;
    unsigned i;
    uint8_t *dest;
    
    result.type = data.type;
    result.len = data.len;
    result.data = dest = apr_palloc(ctx->local_pool, data.len);
    
    for (i = 0; i < data.len; i++)
    {
        dest[i] = data.data[data.len - i - 1u];
    }
    return MAKE_VALUE(BINARY, ctx->local_pool, result);
}

static size_t default_binary_length(exec_context *ctx ATTR_UNUSED, binary_data data)
{
    return data.len * CHAR_BIT;
}

static expr_value default_binary_concat(exec_context *ctx, 
                                        binary_data arg1, binary_data arg2)
{
    binary_data result;
    uint8_t *dest;
    
    result.type = arg1.type;
    result.len = arg1.len + arg2.len;
    result.data = dest = apr_palloc(ctx->local_pool, result.len);
    memcpy(dest, arg1.data, arg1.len);
    memcpy(dest + arg1.len, arg2.data, arg2.len);
    
    return MAKE_VALUE(BINARY, ctx->local_pool, result);
}

static expr_value default_binary_repeat(exec_context *ctx, 
                                        binary_data arg, double num)
{
    binary_data result;
    size_t factor;
    unsigned i;
    uint8_t *dest;
    
    if (num < 1.0)
    {
        return MAKE_VALUE(BINARY, ctx->static_pool,
                          ((binary_data){.type = arg.type,
                                  .data = (const uint8_t *)"",
                                  .len = 0}));
    }                         

    factor = (size_t)num;
    result.type = arg.type;
    result.len = arg.len * factor;
    result.data = dest = apr_palloc(ctx->local_pool, result.len);
    for (i = 0; i < factor; i++)
    {
        memcpy(dest + i * arg.len, arg.data, arg.len);
    }
    return MAKE_VALUE(BINARY, ctx->local_pool, result);
}


static expr_value default_binary_strip(exec_context *ctx, 
                                       binary_data arg1, binary_data suffix)
{
    if (suffix.len > arg1.len)
        return MAKE_VALUE(BINARY, ctx->local_pool, arg1);
    if (memcmp(arg1.data + arg1.len - suffix.len, suffix.data, suffix.len) != 0)
        return MAKE_VALUE(BINARY, ctx->local_pool, arg1);

    arg1.len -= suffix.len;
    return MAKE_VALUE(BINARY, ctx->local_pool, arg1);
}

static expr_value default_binary_split(exec_context *ctx, binary_data data, const expr_value *optarg)
{
    expr_value result;
    unsigned i;

    if (optarg == NULL)
    {
        result = make_expr_value_list(ctx, data.len * CHAR_BIT);
        
        for (i = 0; i < data.len; i++)
        {
            unsigned j = 0;
            
            for (j = 0; j < CHAR_BIT; j++)
            {
                expr_value *val = &APR_ARRAY_IDX(result.v.list, i, expr_value);
                set_expr_value_num(val, 
                                   (data.data[i] & (1 << j)) == 0 ? 0.0 : 1.0);
            }
        }
    }
    else if (optarg->type == VALUE_NUMBER)
    {
        size_t n = (size_t)optarg->v.num;
        size_t chunksize = CHECKED_DIVIDE(ctx, data.len, n);

        result = make_expr_value_list(ctx, n);
        for (i = 0; i < n; i++, data.data += chunksize)
        {
            expr_value *val = &APR_ARRAY_IDX(result.v.list, i, expr_value);
            val->type = VALUE_BINARY;
            val->v.bin.type = &default_binary_data;
            val->v.bin.data = data.data;
            val->v.bin.len = data.len < chunksize ? data.len : chunksize;
            data.len -= val->v.bin.len;
            data.data += val->v.bin.len;
        }
        
    }
    else if (optarg->type == VALUE_BINARY)
    {
        const uint8_t *next;
        result = make_expr_value_list(ctx, 0);

        while ((next = find_binary(data, optarg->v.bin)) != NULL)
        {
            expr_value *val = &APR_ARRAY_PUSH(result.v.list, expr_value);
            val->type = VALUE_BINARY;
            val->scope = ctx->local_pool;
            val->v.bin.type = &default_binary_data;
            val->v.bin.data = data.data;
            if (next == data.data && data.len > 0)
            {
                val->v.bin.len = 1;
                data.len--;
                data.data++;
            }
            else
            {
                val->v.bin.len = (size_t)(next - data.data);
                data.len -= val->v.bin.len + optarg->v.bin.len;
                data.data = next + optarg->v.bin.len;
            }
        }
        if (data.len > 0)
        {
            expr_value *val = &APR_ARRAY_PUSH(result.v.list, expr_value);
            val->type = VALUE_BINARY;
            val->scope = ctx->local_pool;
            val->v.bin = data;
        }
    }
    else
        raise_error(ctx, TEN_BAD_TYPE);
    return result;
}

static expr_value default_binary_tokenize(exec_context *ctx, binary_data data, const expr_value *optarg)
{
    expr_value result;
    unsigned i;

    if (optarg == NULL)
    {
        result = make_expr_value_list(ctx, data.len);
        
        for (i = 0; i < data.len; i++)
        {
            expr_value *val = &APR_ARRAY_IDX(result.v.list, i, expr_value);
            set_expr_value_num(val, (double)data.data[i]);
        }
    }
    else if (optarg->type == VALUE_NUMBER)
    {
        size_t chunksize = (size_t)optarg->v.num;
        size_t n = CHECKED_DIVIDE(ctx, data.len + chunksize - 1, chunksize);

        result = make_expr_value_list(ctx, n);
        for (i = 0; i < n; i++, data.data += chunksize)
        {
            expr_value *val = &APR_ARRAY_IDX(result.v.list, i, expr_value);
            val->type = VALUE_BINARY;
            val->v.bin.type = &default_binary_data;
            val->v.bin.data = data.data;
            val->v.bin.len = data.len < chunksize ? data.len : chunksize;
            data.len -= val->v.bin.len;
            data.data += val->v.bin.len;
        }
        
    }
    else if (optarg->type == VALUE_BINARY)
    {
        const uint8_t *next;
        result = make_expr_value_list(ctx, 0);

        while (data.len > 0 && 
               (next = find_binary((binary_data){.data = data.data + 1, 
                           .len = data.len - 1}, optarg->v.bin)) != NULL)
        {
            expr_value *val = &APR_ARRAY_PUSH(result.v.list, expr_value);
            val->type = VALUE_BINARY;
            val->scope = ctx->local_pool;
            val->v.bin.data = data.data;
            val->v.bin.len = (size_t)(next - data.data);
            data.len -= val->v.bin.len + optarg->v.bin.len;
            data.data = next;
        }
        if (data.len > 0)
        {
            expr_value *val = &APR_ARRAY_PUSH(result.v.list, expr_value);
            val->type = VALUE_BINARY;
            val->scope = ctx->local_pool;
            val->v.bin = data;
        }
    }
    else
        raise_error(ctx, TEN_BAD_TYPE);
    return result;
}

static ATTR_PURE
expr_value default_binary_item(exec_context *ctx, binary_data data, expr_value arg)
{
    size_t idx;

    if (arg.type != VALUE_NUMBER)
        raise_error(ctx, TEN_BAD_TYPE);
    
    idx = normalize_index(ctx, data.len * CHAR_BIT, arg.v.num);

    return MAKE_NUM_VALUE((data.data[idx / CHAR_BIT] & (1 << (idx % CHAR_BIT))) == 0 ? 0.0 : 1.0);
}


static ATTR_PURE
expr_value default_binary_range(exec_context *ctx, binary_data data, expr_value arg1, expr_value arg2)
{
    size_t idx1;
    size_t idx2;

    if (arg1.type != VALUE_NUMBER || arg2.type != VALUE_NUMBER)
        raise_error(ctx, TEN_BAD_TYPE);
    
    idx1 = normalize_index(ctx, data.len * CHAR_BIT, arg1.v.num);
    idx2 = normalize_index(ctx, data.len * CHAR_BIT, arg2.v.num);

    if ((idx1 % CHAR_BIT) != 0 || (idx2 % CHAR_BIT) != 0)
        raise_error(ctx, APR_EINVAL);
    
    idx1 /= CHAR_BIT;
    idx2 /= CHAR_BIT;
    
    data.data += idx1;
    data.len = idx1 > idx2 ? 0u : idx2 - idx1 + 1u;
    return MAKE_VALUE(BINARY, ctx->local_pool, data);
}

static int default_binary_compare(exec_context *ctx ATTR_UNUSED, bool inexact ATTR_UNUSED, 
                                  binary_data data1, binary_data data2, 
                                  bool ordering ATTR_UNUSED)
{
    size_t minlen = data1.len < data2.len ? data1.len : data2.len;
    int rc = memcmp(data1.data, data2.data, minlen);
    if (rc != 0)
        return rc;

    return data1.len > data2.len ? 1 : 
        data1.len < data2.len ? -1 : 0;
}

const binary_data_type default_binary_data = {
    .name = "binary",
    .to_string = default_binary_tostring,
    .to_number = default_binary_tonumber,
    .reverse = default_binary_reverse,
    .length = default_binary_length,
    .concat = default_binary_concat,
    .repeat = default_binary_repeat,
    .strip = default_binary_strip,
    .split = default_binary_split,
    .tokenize = default_binary_tokenize,
    .item = default_binary_item,
    .range = default_binary_range,
    .compare = default_binary_compare
};
