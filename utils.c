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
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>
#include <apr_date.h>
#include "engine.h"

expr_node *make_expr_node(exec_context *ctx, enum expr_type t, ...)
{
    va_list args;
    expr_node *new_expr;

    va_start(args, t);
    
    new_expr = apr_palloc(ctx->static_pool, sizeof(*new_expr));
    new_expr->type = t;
    switch (t) 
    {
        case EXPR_LITERAL:
            new_expr->x.literal = va_arg(args, expr_value);
            assert(apr_pool_is_ancestor(new_expr->x.literal.scope, ctx->static_pool));
            break;
        case EXPR_VARREF:
            new_expr->x.varname = va_arg(args, uint8_t *);
            break;
        case EXPR_LIST:
            new_expr->x.list = va_arg(args, apr_array_header_t *);
            assert(apr_pool_is_ancestor(new_expr->x.list->pool, 
                                        ctx->static_pool));
            break;
        case EXPR_CONTEXT:
            /* nothing */
            break;
        case EXPR_OPERATOR:
        {
            unsigned i;
            
            new_expr->x.op.func = va_arg(args, expr_operator_func);
            new_expr->x.op.nargs = va_arg(args, unsigned);
            assert(new_expr->x.op.nargs <= 3);
            for (i = 0; i < new_expr->x.op.nargs; i++)
                new_expr->x.op.args[i] = va_arg(args, expr_node *);
            break;
        }
        default:
            assert(0);
    }
    va_end(args);
    return new_expr;
}


tree_node *lookup_var(exec_context *ctx, uint8_t *name, bool local)
{
    var_bindings *vars;
    bool uplevel = false;
    
    for (vars = ctx->bindings; 
         vars != NULL && (!uplevel || !local); 
         vars = vars->chain, uplevel = true)
    {
        variable *result;
        
        if (!vars->binding)
            continue;
        result = apr_hash_get(vars->binding, name, APR_HASH_KEY_STRING);
        if (result && (!result->local || !uplevel))
            return result->node;
    }
    return NULL;
}

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
    
    return MAKE_VALUE(TIMESTAMP, NULL, mktime(&parsed));
}

expr_value parse_net_timestamp(exec_context *ctx, const char *ts)
{
    apr_time_t parsed;

    parsed = apr_date_parse_rfc(ts);
    if (parsed == APR_DATE_BAD)
        raise_error(ctx, APR_EBADDATE);
    return MAKE_NUM_VALUE((double)apr_time_sec(parsed));
}

expr_value decode_hex(exec_context *ctx, const char *hex)
{
    
}

double parse_duration(exec_context *ctx, const char *dur)
{
    struct tm collect;

    memset(&collect, 0, sizeof(collect));
    
    while (*dur != '\0' && *dur != 'T')
    {
        unsigned val;
        
        if (!isdigit(*dur))
            raise_error(ctx, APR_FROM_OS_ERROR(EINVAL));
        val = strtol(dur, (char **)&dur, 10);
        switch (*dur) 
        {
            case '\0':
            case 'T':
                /* do nothing */
                break;
            case 'Y':
                collect.tm_year = val;
                break;
            case 'M':
                collect.tm_mon = val;
                break;
            case 'D':
                collect.tm_mday = val + 1;
                break;
            case 'W':
                collect.tm_mday = 7 * val + 1;
                break;
            default:
                raise_error(ctx, APR_FROM_OS_ERROR(EINVAL));
        }
    }
    if (*dur != '\0')
    {
        dur++;
        while (*dur != '\0')
        {
            unsigned val;
            
            if (!isdigit(*dur))
                raise_error(ctx, APR_FROM_OS_ERROR(EINVAL));
            val = strtol(dur, (char **)&dur, 10);
            switch (*dur) 
            {
                case '\0':
                    /* do nothing */
                    break;
                case 'H':
                    collect.tm_hour = val;
                    break;
                case 'M':
                    collect.tm_min = val;
                    break;
                case 'S':
                    collect.tm_sec = val;
                    break;
            }
        }
    }
    return (double)mktime(&collect);
}

extern expr_value unquote_string(exec_context *ctx, const char *str) ATTR_NONNULL;


void raise_error(exec_context *ctx, apr_status_t code)
{
    siglongjmp(*ctx->exitpoint, code);
}

