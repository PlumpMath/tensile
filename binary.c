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

static bool binary_isend(exec_context *ctx ATTR_UNUSED, const void *data)
{
    return ((const binary_data *)data)->len == 0;
}

static expr_value binary_getprefix(exec_context *ctx, void *data, size_t len)
{
    expr_value result;
    binary_data *bin = data;

    if (len > bin->len)
        len = bin->len;

    result = MAKE_VALUE(BINARY, ctx->local_pool, 
                        ((binary_data){.type = bin->type,
                                .data = bin->data,
                                .len = len
                                }));
    
    bin->len -= len;
    bin->data += len;
    return result;
}

static size_t binary_find(exec_context *ctx ATTR_UNUSED, const void *data, const void *sep)
{
    const binary_data *bin = data;
    const binary_data *binsep = sep;
    const uint8_t *next;

    if (bin->len == 0)
        return (size_t)(-1);
    
    next = find_binary(*bin, *binsep);
    if (next == NULL)
        return bin->len;
    
    return (size_t)(next - bin->data);
}

static void binary_skip(exec_context *ctx ATTR_UNUSED, void *data, const void *sep)
{
    binary_data *bin = data;
    const binary_data *binsep = sep;

    if (binsep->len > bin->len)
        return;
    if (memcmp(bin->data, binsep->data, binsep->len) != 0)
        return;
    bin->data += binsep->len;
    bin->len -= binsep->len;
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
                set_expr_value_num(ref_expr_list_value(ctx, result, i),
                                   (data.data[i] & (1 << j)) == 0 ? 0.0 : 1.0);
            }
        }
    }
    else if (optarg->type == VALUE_NUMBER)
    {
        if (optarg->v.num < 1.0)
            raise_error(ctx, APR_EINVAL);

        result = generic_split(ctx, &data, binary_isend, binary_getprefix, 
                               data.len / (size_t)optarg->v.num);
    }
    else if (optarg->type == VALUE_BINARY)
    {

        result = generic_tokenize(ctx, &data, &optarg->v.bin,
                                  binary_find, binary_skip, binary_getprefix);
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
            expr_value *val = ref_expr_list_value(ctx, result, i);
            set_expr_value_num(val, (double)data.data[i]);
        }
    }
    else if (optarg->type == VALUE_NUMBER)
    {
        
        if (optarg->v.num < (double)CHAR_BIT)
            raise_error(ctx, APR_EINVAL);

        result = generic_split(ctx, &data, binary_isend, binary_getprefix,
                               (size_t)optarg->v.num / CHAR_BIT);
    }
    else if (optarg->type == VALUE_BINARY)
    {
        result = generic_tokenize(ctx, &data, &optarg->v.bin,
                                  binary_find, NULL, binary_getprefix);
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

static size_t binary_getlen(exec_context *ctx ATTR_UNUSED, const void *data)
{
    return ((binary_data *)data)->len * CHAR_BIT;
}

static expr_value ATTR_PURE
binary_extract(exec_context *ctx, const void *data, size_t start, size_t len)
{
    const binary_data *bindata = data;

    if ((start % CHAR_BIT) != 0 || (len % CHAR_BIT) != 0)
        raise_error(ctx, APR_EINVAL);

    start /= CHAR_BIT;
    len /= CHAR_BIT;
    
    if (start > bindata->len)
    {
        start = 0;
        len = 0;
    }
    else if (start + len > bindata->len)
    {
        len = bindata->len - start;
    }

    if (len == 0)
    {
        return MAKE_VALUE(BINARY, ctx->static_pool,
                          ((binary_data){.type = bindata->type,
                                  .data = (const uint8_t *)"",
                                  .len = 0}));
    }
    return MAKE_VALUE(BINARY, ctx->local_pool,
                      ((binary_data){.type = bindata->type,
                              .data = bindata->data + start,
                              .len = len}));
}


static ATTR_PURE
expr_value default_binary_range(exec_context *ctx, binary_data data, expr_value arg1, expr_value arg2)
{
    if (arg1.type != VALUE_NUMBER || arg2.type != VALUE_NUMBER)
        raise_error(ctx, TEN_BAD_TYPE);

    return generic_range(ctx, &data, arg1.v.num, arg2.v.num, binary_getlen, binary_extract);
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
