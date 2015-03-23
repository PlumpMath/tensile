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
#include <uniname.h>
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
            
            new_expr->x.op.func = va_arg(args, const operator_info *);
            for (i = 0; i < new_expr->x.op.func->n_args; i++)
            {
                new_expr->x.op.args[i] = va_arg(args, expr_node *);
                assert(new_expr->x.op.args[i] != NULL);
            }
            break;
        }
        case EXPR_AT_CONTEXT:
        case EXPR_DEFAULT:
            new_expr->x.at.arg = va_arg(args, expr_node *);
            new_expr->x.at.context = va_arg(args, expr_node *);
            break;
        case EXPR_TYPECAST:
        {
            typecast_func cast = apr_hash_get(ctx->conf->typecasts, 
                                              va_arg(args, uint8_t *),
                                              APR_HASH_KEY_STRING);
            if (cast == NULL)
                raise_error(ctx, TEN_BAD_TYPE);
            new_expr->x.cast.arg = va_arg(args, expr_node *);
            new_expr->x.cast.func = cast;
            break;
        }
        default:
            assert(0);
    }
    va_end(args);
    return new_expr;
}

tree_node *lookup_var(exec_context *ctx, const uint8_t *name, bool local)
{
    var_bindings *vars;
    bool uplevel = false;
    
    for (vars = ctx->bindings; 
         vars != NULL && (!uplevel || !local); 
         vars = vars->chain, uplevel = true)
    {
        tree_node *result;
        
        if (!vars->binding)
            continue;
        result = apr_hash_get(vars->binding, name, APR_HASH_KEY_STRING);
        if (result && (!result->local || !uplevel))
            return result;
    }
    return NULL;
}

tree_node *define_var(exec_context *ctx, const uint8_t *name)
{
    tree_node *newval = apr_palloc(ctx->bindings->scope, sizeof(*newval));
    
    newval->scope = ctx->bindings->scope;
    newval->attrs = NULL;
    newval->parent = NULL;
    newval->notifier = NULL;
    newval->value = NULL_VALUE;
    newval->local = false;
    
    name = (const uint8_t *)apr_pstrdup(ctx->bindings->scope, (const char *)name);
    if (ctx->bindings->binding == NULL)
        ctx->bindings->binding = apr_hash_make(ctx->bindings->scope);
    apr_hash_set(ctx->bindings->binding, name, APR_HASH_KEY_STRING, newval);
    return newval;
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

uint8_t *find_binary(binary_data data, binary_data search)
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

void augment_arg_list(exec_context *ctx, const action_def *def,
                      apr_array_header_t *args, bool complete)
{
    unsigned i;

    if (!def->vararg && (unsigned)args->nelts > def->n_args)
        raise_error(ctx, TEN_TOO_MANY_ARGS);

    for (i = 0; i < def->n_args; i++)
    {
        const expr_node *defexpr = def->defaults[i];
                
        if (i >= (unsigned)args->nelts)
        {
            if (!complete)
                return;

            if (defexpr == NULL)
                raise_error(ctx, TEN_TOO_MANY_ARGS);
            APR_ARRAY_PUSH(args, const expr_node *) = defexpr;
        }
        else if (APR_ARRAY_IDX(args, i, const expr_node *) == NULL)
        {
            if (defexpr == NULL)
                raise_error(ctx, TEN_NO_REQUIRED_ARG);
            APR_ARRAY_IDX(args, i, const expr_node *) = defexpr;
        }
    }
    for (; i < (unsigned)args->nelts; i++)
    {
        if (APR_ARRAY_IDX(args, i, const expr_node *) == NULL)
            raise_error(ctx, TEN_NO_REQUIRED_ARG);
    }    
}

action *make_action(exec_context *ctx, enum action_type type, ...)
{
    va_list args;
    action *new_act = apr_palloc(ctx->static_pool, sizeof(*new_act));
    
    va_start(args, type);
    new_act->type = type;
    
    switch (type)
    {
        case ACTION_CALL:
        {
            new_act->c.call.def = va_arg(args, const action_def *);
            new_act->c.call.args = va_arg(args, apr_array_header_t *);

            augment_arg_list(ctx, new_act->c.call.def, new_act->c.call.args, true);

            break;
        }
        case ACTION_NOT:
            new_act->c.negated = va_arg(args, action *);
            break;
        case ACTION_SEQ:
            new_act->c.seq = va_arg(args, apr_array_header_t *);
            break;
        case ACTION_ALT:
            new_act->c.alt = va_arg(args, apr_array_header_t *);
            break;
        case ACTION_SCOPE:
            new_act->c.scoped = va_arg(args, action *);
            break;
        case ACTION_CONDITIONAL:
            new_act->c.cond.guard = va_arg(args, action *);
            new_act->c.cond.body = va_arg(args, action *);
            break;
        default:
            assert(0);
            
    }
    va_end(args);

    return new_act;
}


#define ACTION_COMBINATOR(_mode, _type)                                 \
action *_mode##_actions(exec_context *ctx, action *arg1, action *arg2)  \
{                                                                       \
    apr_array_header_t *narr;                                           \
                                                                        \
    if (arg1 == NULL)                                                   \
        return arg2;                                                    \
    if (arg2 == NULL)                                                   \
        return arg1;                                                    \
    if (arg1->type == _type)                                            \
    {                                                                   \
        if (arg2->type == _type)                                        \
            apr_array_cat(arg1->c._mode, arg2->c._mode);                \
        else                                                            \
            APR_ARRAY_PUSH(arg1->c._mode, action *) = arg2;                      \
        return arg1;                                                    \
    }                                                                   \
    if (arg2->type == _type)                                            \
    {                                                                   \
        narr = apr_array_make(ctx->static_pool, 1, sizeof(action *));   \
        APR_ARRAY_IDX(narr, 0, action *) = arg1;                        \
        apr_array_cat(narr, arg2->c._mode);                             \
        arg2->c._mode = narr;                                           \
        return arg2;                                                    \
    }                                                                   \
                                                                        \
    narr = apr_array_make(ctx->static_pool, 2, sizeof(action *));       \
    APR_ARRAY_IDX(narr, 0, action *) = arg1;                            \
    APR_ARRAY_IDX(narr, 1, action *) = arg2;                            \
                                                                        \
    return make_action(ctx, _type, narr);                               \
}                                                                       \
struct action

ACTION_COMBINATOR(seq, ACTION_SEQ);
ACTION_COMBINATOR(alt, ACTION_ALT);

action *make_call(exec_context *ctx, 
                  const action_def *act, 
                  ...) 
{
    apr_array_header_t *argarr = apr_array_make(ctx->static_pool, 0, 
                                                sizeof(expr_node *));
    va_list args;
    const expr_node *next;

    va_start(args, act);
    while ((next = va_arg(args, const expr_node *)) != NULL)
    {
        APR_ARRAY_PUSH(argarr, const expr_node *) = next;
    }
    va_end(args);
    
    return make_action(ctx, ACTION_CALL, act, argarr);
}

const action_def *lookup_action(exec_context *ctx, const uint8_t *name) 
{
    const action_def *found = apr_hash_get(ctx->conf->actions, name, 
                                           APR_HASH_KEY_STRING);
    if (found == NULL)
        raise_error(ctx, APR_ENOENT);

    return found;
}

