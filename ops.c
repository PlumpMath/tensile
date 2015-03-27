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

#include <stdio.h>
#include <uninorm.h>
#include <unicase.h>
#include <unictype.h>
#include <uniwbrk.h>
#include "engine.h"
#include "ops.h"


static const arg_info normal_args[3] = {
    {false, false, true},
    {false, false, true},
    {false, false, true}
};

static const arg_info lvalue_args[3] = {
    {true, true, true},
    {false, false, true},
    {false, false, true}
};


#define DEFINE_OP_INFO_GEN(_name, _kind, _nargs, ...)       \
    const operator_info expr_op_##_name = {                 \
        .n_args = _nargs,                                   \
        .func = (dispatch[]){__VA_ARGS__},                  \
        .info = _kind##_args                                \
    }


#define DEFINE_OP_INFO(_name, _kind, _nargs, _defh, ...)    \
    DEFINE_OP_INFO_GEN(_name, _kind, _nargs, __VA_ARGS__, \
                       {0, _defh, {}})

#define DECLARE_OP_INST(_name, _ctx, _vals)                           \
    static expr_value eval_##_name(exec_context *_ctx,                \
                                   bool __lvalue ATTR_UNUSED,         \
                                   unsigned __nv ATTR_UNUSED,         \
                                   expr_value *_vals)

#define DECLARE_OP_INST1(_name, _type, _ctx, _vals) \
    DECLARE_OP_INST(_name##_##_type, _ctx, _vals)

#define DECLARE_OP_INST2(_name, _type1, _type2, _ctx, _vals)    \
    DECLARE_OP_INST(_name##_##_type1##_##_type2, _ctx, _vals)

#define DISP1(_type, _handler)                          \
    {1, (eval_##_handler##_##_type), {VALUE_##_type,}}
#define DISP2(_type1, _type2, _handler)                                 \
    {2, (eval_##_handler##_##_type1##_##_type2),                        \
        {VALUE_##_type1, VALUE_##_type2,}}

#define VALUE_NONE VALUE_NULL

DECLARE_OP_INST(noop, ctx ATTR_UNUSED, vals)
{
    return vals[0];
}

#define eval_noop_NONE eval_noop

DEFINE_OP_INFO_GEN(noop, normal, 1, {0, eval_noop, {}});
DEFINE_OP_INFO_GEN(lvalue, lvalue, 1, {0, eval_noop, {}});

DECLARE_OP_INST1(uplus, NUMBER, ctx ATTR_UNUSED, vals)
{
    vals[0].v.num = fabs(vals[0].v.num);
    return vals[0];
}

DECLARE_OP_INST1(uplus, TIMESTAMP, ctx ATTR_UNUSED, vals)
{
    tzset();
    vals[0].v.ts += timezone;
    return vals[0];
}

DECLARE_OP_INST1(normalize, STRING, ctx, vals)
{
    size_t len = 0;
    uint8_t *buffer = NULL;
    uint8_t *dest;
    
    buffer = u8_normalize(UNINORM_NFKC, vals[0].v.str, 
                          u8_strlen(vals[0].v.str), 
                          NULL, &len);
    if (buffer == NULL)
        raise_error(ctx, APR_FROM_OS_ERROR(errno));
    dest = apr_palloc(ctx->local_pool, len + 1);
    memcpy(dest, buffer, len);
    dest[len] = '\0';
    vals[0].v.str = dest;
    
    vals[0].scope = ctx->local_pool;
    free(buffer);

    return vals[0];
}

DECLARE_OP_INST1(squeeze, LIST, ctx, vals)
{
    expr_value result = make_expr_value_list(ctx, 0);
    unsigned i;
    expr_value *prev = NULL;
    expr_value *current;
    
    for (i = 0; i < (unsigned)vals[0].v.list->nelts; i++)
    {
        current = ref_expr_list_value(ctx, vals[0], i);
        if (prev == NULL ||
            compare_values(ctx, false, 
                           *prev,
                           *current,
                           false) != 0)
        {
            add_expr_list_value(ctx, &result, *current);
        }
        prev = current;
    }
    return result;
}

DECLARE_OP_INST1(uplus, BINARY, ctx, vals)
{
    if (!vals[0].v.bin.type->uplus)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->uplus(ctx, vals[0].v.bin);
}

DEFINE_OP_INFO(uplus, normal, 1, NULL,
               DISP1(NONE, noop),
               DISP1(NUMBER, uplus),
               DISP1(TIMESTAMP, uplus),
               DISP1(STRING, normalize),
               DISP1(LIST, squeeze),
               DISP1(BINARY, uplus));

DECLARE_OP_INST1(uminus, NUMBER, ctx ATTR_UNUSED, vals)
{
    vals[0].v.num = -vals[0].v.num;
    return vals[0];
}

DECLARE_OP_INST1(reverse, STRING, ctx ATTR_UNUSED, vals)
{
    size_t srclen = u8_strlen(vals[0].v.str);
    uint8_t *result = apr_palloc(ctx->local_pool, srclen + 1);
    const uint8_t *src = vals[0].v.str;
    uint8_t *dest = result + srclen;
    
    *dest = '\0';
    for (;;)
    {
        int len = u8_strmblen(src);
        
        if (len < 0)
            raise_error(ctx, APR_FROM_OS_ERROR(errno));
        if (len == 0)
            break;
        memcpy(dest - len, src, (size_t)len);
        
        dest -= len;
        src += len;
    };

    return MAKE_VALUE(STRING, ctx->local_pool, result);
}

DECLARE_OP_INST1(uminus, TIMESTAMP, ctx ATTR_UNUSED, vals)
{
    tzset();
    vals[0].v.ts -= timezone;
    return vals[0];
}

DECLARE_OP_INST1(reverse, BINARY, ctx, vals)
{
    if (!vals[0].v.bin.type->reverse)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->reverse(ctx, vals[0].v.bin);
}

DECLARE_OP_INST1(reverse, LIST, ctx, vals)
{
    expr_value listval = make_expr_value_list(ctx,
                                              (size_t)vals[0].v.list->nelts);
    unsigned i;
    
    for (i = 0; i < (unsigned)vals[0].v.list->nelts; i++)
    {
        set_expr_list_value(ctx, &listval, i,
                            *ref_expr_list_value(ctx, vals[0],
                                                 (unsigned)vals[0].v.list->nelts - i - 1u));
    }
    return listval;
}

DECLARE_OP_INST1(negate, CLOSURE, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_negate, 1);
    
    set_closure_arg(ctx, &clos, 0, vals[0]);
    return clos;
}

DEFINE_OP_INFO(uminus, normal, 1, NULL,
               DISP1(NONE, noop),
               DISP1(NUMBER, uminus),
               DISP1(TIMESTAMP, uminus),
               DISP1(STRING, reverse),
               DISP1(BINARY, reverse),
               DISP1(LIST, reverse),
               DISP1(CLOSURE, negate));

static inline ATTR_PURE
expr_value eval_typeof(exec_context *ctx, bool lvalue ATTR_UNUSED, 
                       unsigned nv ATTR_UNUSED, expr_value *vals)
{
    static const char* typenames[VALUE_CLOSURE + 1] = {
        [VALUE_NULL] = "null",
        [VALUE_NUMBER] = "number",
        [VALUE_STRING] = "string",
        [VALUE_TIMESTAMP] = "timestamp",
        [VALUE_LIST] = "list",
        [VALUE_CLOSURE] = "closure"
    };
    const char *name;
    
    if (vals[0].type == VALUE_BINARY)
        name = vals[0].v.bin.type->name;
    else
        name = typenames[vals[0].type];
    assert(name != NULL);
    
    return MAKE_VALUE(STRING, ctx->static_pool, (const uint8_t *)name);
}

DEFINE_OP_INFO_GEN(typeof, normal, 1, {0, eval_typeof, {}});

DECLARE_OP_INST1(length, NONE, ctx ATTR_UNUSED, vals ATTR_UNUSED)
{
    return MAKE_NUM_VALUE(0.0);
}

DECLARE_OP_INST1(length, NUMBER, ctx, vals)
{
    if (vals[0].v.num == 0)
        raise_error(ctx, APR_EINVAL);
    
    return MAKE_NUM_VALUE(log10(fabs(vals[0].v.num)));
}

DECLARE_OP_INST1(length, STRING, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE((double)u8_mbsnlen(vals[0].v.str, 
                                             u8_strlen(vals[0].v.str)));
}

DECLARE_OP_INST1(length, LIST, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE((double)vals[0].v.list->nelts);
}

DECLARE_OP_INST1(length, BINARY, ctx, vals)
{
    if (vals[0].v.bin.type->length == NULL)
        raise_error(ctx, APR_ENOTIMPL);
    return MAKE_NUM_VALUE((double)vals[0].v.bin.type->length(ctx, vals[0].v.bin));
}

DEFINE_OP_INFO(length, normal, 1, NULL,
               DISP1(NONE, length),
               DISP1(NUMBER, length),
               DISP1(STRING, length),
               DISP1(BINARY, length),
               DISP1(LIST, length));

DECLARE_OP_INST1(round, NUMBER, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE(round(vals[0].v.num));
}


DECLARE_OP_INST1(strxfrm, STRING, ctx, vals)
{
    const char *lang = uc_locale_language();
    size_t destlen = 0;
    char *xfrm = u8_casexfrm(vals[0].v.str, 
                             u8_strlen(vals[0].v.str), lang,
                             UNINORM_NFKC, NULL, &destlen);
    uint8_t *dest;
    
    if (xfrm == NULL)
        raise_error(ctx, APR_FROM_OS_ERROR(errno));
    dest = apr_palloc(ctx->local_pool, destlen);
    memcpy(dest, xfrm, destlen);
    dest[destlen] = '\0';
    free(xfrm);
    return MAKE_VALUE(STRING, ctx->local_pool, dest);
}

DECLARE_OP_INST1(justdate, TIMESTAMP, ctx ATTR_UNUSED, vals)
{
    return MAKE_VALUE(TIMESTAMP, vals[0].scope,
                      timestamp_just_date(vals[0].v.ts));
}

DECLARE_OP_INST1(loosen, BINARY, ctx, vals)
{
    if (vals[0].v.bin.type->loosen == NULL)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->loosen(ctx, vals[0].v.bin);
}

DECLARE_OP_INST1(loosen, CLOSURE, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_loosen, 1);
    
    set_closure_arg(ctx, &clos, 0, vals[0]);
    return clos;
}

DEFINE_OP_INFO(loosen, normal, 1, NULL,
               DISP1(NONE, noop),
               DISP1(NUMBER, round),
               DISP1(TIMESTAMP, justdate),
               DISP1(STRING, strxfrm),
               DISP1(BINARY, loosen),
               DISP1(CLOSURE, loosen));


enum top_char_class {
    CC_NONE,
    CC_ALPHA,
    CC_NUMBER,
    CC_PUNCT,
    CC_SYMBOL,
    CC_SEPARATOR,
    CC_OTHER
};

static inline enum top_char_class get_top_char_class(ucs4_t ch)
{
    if (uc_is_general_category(ch, UC_LETTER) ||
        uc_is_general_category(ch, UC_MARK))
        return CC_ALPHA;
    if (uc_is_general_category(ch, UC_NUMBER))
        return CC_NUMBER;
    if (uc_is_general_category(ch, UC_PUNCTUATION))
        return CC_PUNCT;
    if (uc_is_general_category(ch, UC_SYMBOL))
        return CC_SYMBOL;
    if (uc_is_general_category(ch, UC_SEPARATOR))
        return CC_SEPARATOR;
    return CC_OTHER;
}

DECLARE_OP_INST1(frac, NUMBER, ctx ATTR_UNUSED, vals)
{
    double ipart;
            
    return MAKE_NUM_VALUE(modf(vals[0].v.num, &ipart));
}

DECLARE_OP_INST1(tokenize, STRING, ctx, vals)
{
    const uint8_t *iter = vals[0].v.str;
    const uint8_t *start = iter;
    enum top_char_class prev = CC_NONE;
    expr_value list = make_expr_value_list(ctx, 0);
    uint8_t *token;
    
    for (;;) {
        ucs4_t ch;
        enum top_char_class cur;
        
        iter = u8_next(&ch, iter);
        if (iter == NULL)
            break;
        cur = get_top_char_class(ch);
        if (cur != prev)
        {
            if (start != iter)
            {
                token = (uint8_t *)apr_pstrndup(ctx->local_pool, 
                                                (const char *)start, 
                                                (apr_size_t)(iter - start));
                add_expr_list_value(ctx, &list, 
                                    MAKE_VALUE(STRING, 
                                               ctx->local_pool, token));
                start = iter;
            }
            prev = cur;
        }
    }
    if (*start)
    {
        token = (uint8_t *)apr_pstrdup(ctx->local_pool, (const char *)start);
        add_expr_list_value(ctx, &list, 
                            MAKE_VALUE(STRING, ctx->local_pool, token));
    }
    return list;
}

DECLARE_OP_INST1(tokenize1, BINARY, ctx, vals)
{
    if (vals[0].v.bin.type->split == NULL)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->tokenize(ctx, vals[0].v.bin, NULL);
}

DEFINE_OP_INFO(frac, normal, 1, NULL,
               DISP1(NONE, noop),
               DISP1(NUMBER, frac),
               DISP1(STRING, tokenize),
               DISP1(BINARY, tokenize1));

DECLARE_OP_INST1(floor, NUMBER, ctx ATTR_UNUSED, vals)
{
    double ipart;
    modf(vals[0].v.num, &ipart);
    return MAKE_NUM_VALUE(ipart);
}

DECLARE_OP_INST1(wordsplit, STRING, ctx, vals)
{
    size_t len = u8_strlen(vals[0].v.str);
    char *breaks = apr_palloc(ctx->local_pool, len + 1);
    unsigned i;
    unsigned wordstart = 0;
    expr_value list = make_expr_value_list(ctx, 0);
            
    breaks[len] = 1;
    u8_wordbreaks(vals[0].v.str, len, breaks);
    
    for (i = 0; i <= len; i++)
    {
        if (breaks[i])
        {
            add_expr_list_value(ctx, &list,
                                MAKE_VALUE(STRING, ctx->local_pool,
                                           (const uint8_t *)
                                           apr_pstrndup(ctx->local_pool,
                                                        (const char *)
                                                        (vals[0].v.str + wordstart),
                                                        i - wordstart)));
            wordstart = i;
        }
    }
    return list;
}

DECLARE_OP_INST1(split1, BINARY, ctx, vals)
{
    if (vals[0].v.bin.type->tokenize == NULL)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->tokenize(ctx, vals[0].v.bin, NULL);
}


DEFINE_OP_INFO(floor, normal, 1, NULL,
               DISP1(NONE, noop),
               DISP1(NUMBER, floor),
               DISP1(STRING, wordsplit),
               DISP1(BINARY, split1));


DECLARE_OP_INST1(parent, NODEREF, ctx ATTR_UNUSED, vals)
{
    if (!vals[0].v.ref->parent)
        return NULL_VALUE;
    
    return MAKE_VALUE(NODEREF,
                      vals[0].v.ref->parent->scope,
                      vals[0].v.ref->parent);
}

DEFINE_OP_INFO(parent, lvalue, 1, NULL,
               DISP1(NONE, noop),
               DISP1(NODEREF, parent));


DECLARE_OP_INST1(count, NODEREF, ctx ATTR_UNUSED, vals)
{
    if (!vals[0].v.ref->attrs)
        return MAKE_NUM_VALUE(0.0);
    return MAKE_NUM_VALUE((double)apr_hash_count(vals[0].v.ref->attrs));
}

DEFINE_OP_INFO(count, lvalue, 1, NULL,
               DISP1(NONE, length),
               DISP1(NODEREF, count));


DECLARE_OP_INST1(emptylist, NONE, ctx, vals ATTR_UNUSED)
{
    return make_expr_value_list(ctx, 0);
}

DECLARE_OP_INST1(children, NODEREF, ctx, vals)
{
    expr_value list = make_expr_value_list(ctx, 0);
    apr_hash_index_t *iter;

    if (!vals[0].v.ref->attrs)
        return list;

    for (iter = apr_hash_first(ctx->local_pool, vals[0].v.ref->attrs);
         iter != NULL;
         iter = apr_hash_next(iter))
    {
        expr_value pair = make_expr_value_list(ctx, 2);
        const uint8_t *key;
        tree_node *child;

        apr_hash_this(iter, (const void **)&key, NULL, (void **)&child);
        set_expr_list_value(ctx, &pair, 0, 
                            MAKE_VALUE(STRING, vals[0].v.ref->scope, key));
        set_expr_list_value(ctx, &pair, 1,
                            MAKE_VALUE(NODEREF, vals[0].v.ref->scope, child));
        add_expr_list_value(ctx, &list, pair);
    }
    return list;
}

DEFINE_OP_INFO(children, lvalue, 1, NULL,
               DISP1(NONE, emptylist),
               DISP1(NODEREF, children));

DECLARE_OP_INST2(plus, NUMBER, NUMBER, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE(vals[0].v.num + vals[1].v.num);
}

DECLARE_OP_INST2(append, STRING, STRING, ctx, vals)
{
    return MAKE_VALUE(STRING, ctx->local_pool, 
                      (const uint8_t *)
                      apr_pstrcat(ctx->local_pool,
                                  vals[0].v.str, vals[1].v.str, NULL));
}

DECLARE_OP_INST2(plus, TIMESTAMP, NUMBER, ctx ATTR_UNUSED, vals)
{
    return MAKE_VALUE(TIMESTAMP, vals[0].scope,
                      vals[0].v.ts + (time_t)vals[0].v.num);
}

DECLARE_OP_INST2(append, BINARY, BINARY, ctx ATTR_UNUSED, vals)
{
    if (!vals[0].v.bin.type->concat)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->concat(ctx, vals[0].v.bin, vals[1].v.bin);
}

DECLARE_OP_INST2(append, LIST, LIST, ctx, vals)
{
    expr_value result;
    unsigned i;
    
    result = make_expr_value_list(ctx, (size_t)vals[0].v.list->nelts +
                                  (size_t)vals[0].v.list->nelts);
    for (i = 0; i < (unsigned)vals[0].v.list->nelts; i++)
    {
        set_expr_list_value(ctx, &result, i,
                            *ref_expr_list_value(ctx, vals[0], i));
    }
    for (i = 0; i < (unsigned)vals[1].v.list->nelts; i++)
    {
        set_expr_list_value(ctx, &result,
                            i + (unsigned)vals[0].v.list->nelts,
                            *ref_expr_list_value(ctx, vals[1], i));
    }
    return result;
}

DECLARE_OP_INST2(alt, CLOSURE, CLOSURE, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_alt, 2);
    
    set_closure_arg(ctx, &clos, 0, vals[0]);
    set_closure_arg(ctx, &clos, 1, vals[1]);
    return clos;
}

DEFINE_OP_INFO(plus, normal, 2, NULL,
               DISP2(NUMBER, NUMBER, plus),
               DISP2(STRING, STRING, append),
               DISP2(TIMESTAMP, NUMBER, plus),
               DISP2(BINARY, BINARY, append),
               DISP2(LIST, LIST, append),
               DISP2(CLOSURE, CLOSURE, alt));


DECLARE_OP_INST2(minus, NUMBER, NUMBER, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE(vals[0].v.num - vals[1].v.num);
}

DECLARE_OP_INST2(strip, STRING, STRING, ctx, vals)
{
    if (!u8_endswith(vals[0].v.str, vals[1].v.str))
        return vals[0];
            
    return MAKE_VALUE(STRING, ctx->local_pool,
                      (const uint8_t *)
                      apr_pstrndup(ctx->local_pool,
                                   (const char *)vals[0].v.str,
                                   u8_strlen(vals[0].v.str) -
                                   u8_strlen(vals[1].v.str)));
}

DECLARE_OP_INST2(minus, TIMESTAMP, NUMBER, ctx ATTR_UNUSED, vals)
{
    return MAKE_VALUE(TIMESTAMP, vals[0].scope,
                      vals[0].v.ts - (time_t)vals[1].v.num);
}

DECLARE_OP_INST2(diff, TIMESTAMP, TIMESTAMP, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE(difftime(vals[0].v.ts, vals[1].v.ts));
}

DECLARE_OP_INST2(strip, BINARY, BINARY, ctx, vals)
{
    if (!vals[0].v.bin.type->strip)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->strip(ctx, vals[0].v.bin, vals[1].v.bin);
}


DECLARE_OP_INST2(strip, LIST, LIST, ctx, vals)
{
    expr_value result;
    unsigned i;
    
    if (!is_list_suffix(ctx, vals[0], vals[1]))
        return vals[1];
    
    result = make_expr_value_list(ctx, (size_t)vals[0].v.list->nelts -
                                  (size_t)vals[1].v.list->nelts);
    for (i = 0; i < (unsigned)result.v.list->nelts; i++)
    {
        set_expr_list_value(ctx, &result, i,
                            *ref_expr_list_value(ctx, vals[0], i));
    }
    return result;
}

DECLARE_OP_INST2(andnot, CLOSURE, CLOSURE, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_and_not, 2);
    
    set_closure_arg(ctx, &clos, 0, vals[0]);
    set_closure_arg(ctx, &clos, 1, vals[1]);
    return clos;
}

DEFINE_OP_INFO(minus, normal, 2, NULL,
               DISP2(NUMBER, NUMBER, minus),
               DISP2(STRING, STRING, strip),
               DISP2(TIMESTAMP, NUMBER, minus),
               DISP2(TIMESTAMP, TIMESTAMP, diff),
               DISP2(BINARY, BINARY, strip),
               DISP2(LIST, LIST, strip),
               DISP2(CLOSURE, CLOSURE, andnot));


DECLARE_OP_INST2(mul, NUMBER, NUMBER, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE(vals[0].v.num * vals[1].v.num);
}

DECLARE_OP_INST2(repeat, STRING, NUMBER, ctx, vals)
{
    size_t factor;
    size_t srclen;
    uint8_t *buf;
    unsigned i;
    
    if (vals[1].v.num < 1.0)
        return MAKE_VALUE(STRING, ctx->static_pool, (const uint8_t *)"");
    if (vals[1].v.num == 1.0)
        return vals[0];
    
    factor = (size_t)vals[1].v.num;
    srclen = u8_strlen(vals[0].v.str);
    buf = apr_palloc(ctx->local_pool, factor * srclen + 1);
    buf[factor * srclen] = '\0';
    
    for (i = 0; i < factor; i++)
    {
        memcpy(buf + i * srclen, vals[0].v.str, srclen);
    }
    return MAKE_VALUE(STRING, ctx->local_pool, buf);
}

DECLARE_OP_INST2(intersperse, STRING, STRING, ctx, vals)
{
    size_t len = u8_strlen(vals[0].v.str);
    size_t ilen = u8_strlen(vals[1].v.str);
    size_t nchars = u8_mbsnlen(vals[0].v.str, u8_strlen(vals[0].v.str));
    uint8_t *dest = apr_palloc(ctx->local_pool,
                               len + nchars * ilen + 1);
    const uint8_t *src;
    int ucslen;
    expr_value result = MAKE_VALUE(STRING, ctx->local_pool, dest);
    
    for (src = vals[0].v.str; *src; src += ucslen, dest += (size_t)ucslen + ilen)
    {
        ucslen = u8_strmblen(src);
        if (ucslen < 0)
            raise_error(ctx, APR_FROM_OS_ERROR(errno));
        
        memcpy(dest, src, (size_t)ucslen);
        memcpy(dest + ucslen, vals[1].v.str, ilen);
    }
    *dest = '\0';

    return result;
}


DECLARE_OP_INST2(repeat, BINARY, NUMBER, ctx, vals)
{
    if (!vals[0].v.bin.type->repeat)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->repeat(ctx, vals[0].v.bin, vals[1].v.num);
}

DECLARE_OP_INST2(intersperse, BINARY, BINARY, ctx, vals)
{
    if (!vals[0].v.bin.type->intersperse)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->intersperse(ctx, vals[0].v.bin, vals[1].v.bin);
}

DECLARE_OP_INST2(repeat, LIST, NUMBER, ctx, vals)
{
    expr_value result;
    size_t factor;
    unsigned i;
    
    if (vals[1].v.num < 1.0)
        return make_expr_value_list(ctx, 0);
    
    if (vals[1].v.num == 1.0)
        return vals[0];

    factor = (size_t)vals[1].v.num;

    result = make_expr_value_list(ctx, (unsigned)vals[0].v.list->nelts * factor);

    for (i = 0; i < factor; i++)
    {
        unsigned j;

        for (j = 0; j < (unsigned)vals[0].v.list->nelts; j++)
        {
            set_expr_list_value(ctx, &result, 
                                i * (unsigned)vals[0].v.list->nelts + j,
                                *ref_expr_list_value(ctx, vals[0], j));
        }
    }
    return result;
}

DECLARE_OP_INST2(intersperse, LIST, LIST, ctx, vals)
{
    expr_value result = make_expr_value_list(ctx,
                                             (unsigned)vals[0].v.list->nelts *
                                             ((unsigned)vals[1].v.list->nelts + 1u));
    
    unsigned i;
    unsigned j;
    
    for (i = 0; i < (unsigned)vals[0].v.list->nelts; i++)
    {
        set_expr_list_value(ctx, &result,
                            i * ((unsigned)vals[1].v.list->nelts + 1u),
                            *ref_expr_list_value(ctx, vals[0], i));
        for (j = 0; j < (unsigned)vals[1].v.list->nelts; j++)
        {
            set_expr_list_value(ctx, &result,
                                i * ((unsigned)vals[1].v.list->nelts + 1u) + j + 1u,
                                *ref_expr_list_value(ctx, vals[1], j));
        }
    }
    return result;
}

DECLARE_OP_INST2(concat, LIST, STRING, ctx, vals)
{
    unsigned i;
    size_t inlen = u8_strlen(vals[1].v.str);
    size_t outlen = 0u;
    uint8_t *result;
    uint8_t *dest;
    
    for (i = 0; i < (unsigned)vals[0].v.list->nelts; i++)
    {
        const expr_value *v = ref_expr_list_value(ctx, vals[0], i);
        if (v->type != VALUE_STRING)
            raise_error(ctx, TEN_BAD_TYPE);
        outlen += u8_strlen(v->v.str) + (i ? inlen : 0);
    }
    dest = result = apr_palloc(ctx->local_pool, outlen + 1);
    for (i = 0; i < (unsigned)vals[0].v.list->nelts; i++)
    {
        const expr_value *v = ref_expr_list_value(ctx, vals[0], i);
        
        if (i != 0)
        {
            memcpy(dest, vals[1].v.str, inlen);
            dest += inlen;
        }
        dest = u8_stpcpy(dest, v->v.str);
    }
    return MAKE_VALUE(STRING, ctx->local_pool, result);
}


DECLARE_OP_INST2(and, CLOSURE, CLOSURE, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_and, 2);

    set_closure_arg(ctx, &clos, 0, vals[0]);
    set_closure_arg(ctx, &clos, 1, vals[1]);
    return clos;
}

DEFINE_OP_INFO(mul, normal, 2, NULL,
               DISP2(NUMBER, NUMBER, mul),
               DISP2(STRING, NUMBER, repeat),
               DISP2(STRING, STRING, intersperse),
               DISP2(BINARY, NUMBER, repeat),
               DISP2(BINARY, BINARY, intersperse),
               DISP2(LIST, NUMBER, repeat),
               DISP2(LIST, LIST, intersperse),
               DISP2(LIST, STRING, concat),
               DISP2(CLOSURE, CLOSURE, and));


ATTR_PURE
DECLARE_OP_INST2(div, NUMBER, NUMBER, ctx, vals)
{
    if (vals[1].v.num == 0.0)
        raise_error(ctx, APR_EINVAL);
    
    return MAKE_NUM_VALUE(vals[0].v.num / vals[1].v.num);
}

static bool string_isend(exec_context *ctx ATTR_UNUSED, const void *data)
{
    return *(const uint8_t **)data == '\0';
}

static expr_value string_getprefix(exec_context *ctx, void *data, size_t len)
{
    const uint8_t **pstr = data;
    
    return extract_substr(ctx, *pstr, 0, len, pstr);
}

static size_t string_find(exec_context *ctx ATTR_UNUSED, const void *data, const void *sep)
{
    const uint8_t * const *pstr = data;
    const uint8_t *sepstr = sep;
    const uint8_t *next;
    
    if (**pstr == '\0')
        return (size_t)(-1);
    next = u8_strstr(*pstr, sepstr);
    if (next == NULL)
        return u8_strlen(*pstr);
    
    return (size_t)(next - *pstr);
}

static void string_skip(exec_context *ctx ATTR_UNUSED, void *data, const void *sep)
{
    const uint8_t **pstr = data;
    const uint8_t *sepstr = sep;
    
    if (!u8_startswith(*pstr, sepstr))
        return;
    
    (*pstr) += u8_strlen(sepstr);
}

DECLARE_OP_INST2(split, STRING, NUMBER, ctx, vals)
{
    const uint8_t *str = vals[0].v.str;
    size_t nchar = u8_mbsnlen(str, u8_strlen(str));

    if (vals[1].v.num < 1.0)
        raise_error(ctx, APR_EINVAL);

    return generic_split(ctx, &str, 
                         string_isend,
                         string_getprefix,
                         nchar / (size_t)vals[1].v.num);
}

DECLARE_OP_INST2(split, STRING, STRING, ctx, vals)
{
    const uint8_t *str = vals[0].v.str;
    return generic_tokenize(ctx, &str, vals[1].v.str,
                            string_find,
                            string_skip,
                            string_getprefix);
}

static size_t string_find_prop(exec_context *ctx, const void *data, const void *sep)
{
    const uint8_t *str = *(const uint8_t * const *)data;
    const closure *cls = sep;
    expr_value curchar;
    size_t count;

    if (*str == '\0')
        return (size_t)(-1);

    for (count = 0; *str != '\0';  count++)
    {
        curchar = extract_substr(ctx, str, 0, 1, &str);
        if (call_predicate(ctx, cls, 1, &curchar))
            break;
    }
    return count;
}

static void string_skip_prop(exec_context *ctx, void *data, const void *sep)
{
    const uint8_t *str = *(const uint8_t **)data;
    const closure *cls = sep;
    expr_value curchar;

    if (*str == '\0')
        return;
    
    curchar = extract_substr(ctx, str, 0, 1, &str);
    if (call_predicate(ctx, cls, 1, &curchar))
        *(const uint8_t **)data = str;
}


DECLARE_OP_INST2(split, STRING, CLOSURE, ctx, vals)
{
    const uint8_t *str = vals[0].v.str;
    return generic_tokenize(ctx, &str, &vals[1].v.cls,
                            string_find_prop,
                            string_skip_prop,
                            string_getprefix);
}

DECLARE_OP_INST1(split, BINARY, ctx, vals)
{
    if (!vals[0].v.bin.type->split)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->split(ctx, vals[0].v.bin, &vals[1]);
}

typedef struct list_position {
    expr_value *list;
    size_t pos;
} list_position;

static bool list_isend(exec_context *ctx ATTR_UNUSED, const void *data)
{
    const list_position *pos = data;
    return pos->pos >= (unsigned)pos->list->v.list->nelts;
}

static expr_value list_getprefix(exec_context *ctx, void *data, size_t len)
{
    list_position *pos = data;
    expr_value result;

    result = extract_sublist(ctx, *pos->list, pos->pos, len);
    pos->pos += len;
    return result;
}

static size_t list_find(exec_context *ctx, const void *data, const void *sep)
{
    list_position pos = *(const list_position *)data;
    const expr_value *lsep = sep;
    size_t result;

    if (pos.pos >= (unsigned)pos.list->v.list->nelts)
        return (size_t)(-1);

    result = find_in_list(ctx, *pos.list, *lsep, pos.pos);
    return result == (size_t)(-1) ? 
        (unsigned)pos.list->v.list->nelts - pos.pos : 
        result - pos.pos;
}

static void list_skip(exec_context *ctx, void *data, const void *sep)
{
    list_position pos = *(const list_position *)data;
    const expr_value *lsep = sep;

    if (!is_list_prefix(ctx, *pos.list, *lsep, pos.pos))
        return;
    
    pos.pos += (unsigned)lsep->v.list->nelts;
}

DECLARE_OP_INST2(split, LIST, NUMBER, ctx ATTR_UNUSED, vals)
{
    list_position pos = {&vals[0], 0};

    if (vals[1].v.num < 1.0)
        raise_error(ctx, APR_EINVAL);

    return generic_split(ctx, &pos, 
                         list_isend,
                         list_getprefix,
                         (size_t)vals[0].v.list->nelts / (size_t)vals[1].v.num);
}

DECLARE_OP_INST2(split, LIST, LIST, ctx ATTR_UNUSED, vals)
{
    list_position pos = {&vals[0], 0};
    return generic_tokenize(ctx, &pos, &vals[1],
                            list_find,
                            list_skip,
                            list_getprefix);
}

static size_t list_find_prop(exec_context *ctx, const void *data, const void *sep)
{
    list_position pos = *(const list_position *)data;
    const closure *cls = sep;
    size_t count;

    if (pos.pos >= (unsigned)pos.list->v.list->nelts)
        return (size_t)(-1);

    for (count = 0; pos.pos + count < (unsigned)pos.list->v.list->nelts;  count++)
    {
        if (call_predicate(ctx, cls, 1, 
                           ref_expr_list_value(ctx, *pos.list, 
                                               pos.pos + count)))
            break;
    }
    return count;
}

static void list_skip_prop(exec_context *ctx, void *data, const void *sep)
{
    list_position *pos = data;
    const closure *cls = sep;

    if (pos->pos >= (unsigned)pos->list->v.list->nelts)
        return;

    if (call_predicate(ctx, cls, 1, 
                       ref_expr_list_value(ctx, *pos->list, pos->pos)))
    {
        pos->pos++;
    }
}


DECLARE_OP_INST2(split, LIST, CLOSURE, ctx, vals)
{
    list_position pos = {&vals[0], 0};
    return generic_tokenize(ctx, &pos, &vals[1].v.cls,
                            list_find_prop,
                            list_skip_prop,
                            list_getprefix);
}

DECLARE_OP_INST2(impl, CLOSURE, CLOSURE, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_impl, 2);
    
    set_closure_arg(ctx, &clos, 0, vals[0]);
    set_closure_arg(ctx, &clos, 1, vals[1]);
    return clos;
}

DEFINE_OP_INFO(div, normal, 2, NULL,
               DISP2(NUMBER, NUMBER, div),
               DISP2(STRING, NUMBER, split),
               DISP2(STRING, STRING, split),
               DISP2(STRING, CLOSURE, split),
               DISP1(BINARY, split),
               DISP2(LIST, NUMBER, split),
               DISP2(LIST, LIST, split),
               DISP2(LIST, CLOSURE, split),
               DISP2(CLOSURE, CLOSURE, impl));

ATTR_PURE
DECLARE_OP_INST2(rem, NUMBER, NUMBER, ctx, vals)
{
    if (vals[1].v.num == 0.0)
        raise_error(ctx, APR_EINVAL);
    
    return MAKE_NUM_VALUE(fmod(vals[0].v.num, vals[1].v.num));
}

DECLARE_OP_INST2(tokenize, STRING, NUMBER, ctx, vals)
{
    const uint8_t *str = vals[0].v.str;
    if (vals[1].v.num < 1.0)
        raise_error(ctx, APR_EINVAL);

    return generic_split(ctx, &str, 
                         string_isend,
                         string_getprefix,
                         (size_t)vals[1].v.num);
}

DECLARE_OP_INST2(tokenize, STRING, STRING, ctx, vals)
{
    const uint8_t *str = vals[0].v.str;
    return generic_tokenize(ctx, &str, vals[1].v.str,
                            string_find,
                            NULL,
                            string_getprefix);
}

static size_t string_find_trans(exec_context *ctx, const void *data, const void *sep)
{
    const uint8_t *str = *(const uint8_t * const *)data;
    const closure *cls = sep;
    expr_value curchars[2];
    size_t count;

    if (*str == '\0')
        return (size_t)(-1);

    for (count = 0; *str != '\0';  count++)
    {
        curchars[0] = extract_substr(ctx, str, 0, 1, &str);
        if (*str == '\0')
            break;
        curchars[1] = extract_substr(ctx, str, 0, 1, NULL);
        if (call_predicate(ctx, cls, 2, curchars))
            break;
    }
    return count;
}


DECLARE_OP_INST2(tokenize, STRING, CLOSURE, ctx, vals)
{
    const uint8_t *str = vals[0].v.str;
    return generic_tokenize(ctx, &str, &vals[1].v.cls,
                            string_find_trans,
                            NULL,
                            string_getprefix);
}

DECLARE_OP_INST1(tokenize, BINARY, ctx, vals)
{
    if (!vals[0].v.bin.type->tokenize)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->tokenize(ctx, vals[0].v.bin, &vals[1]);
}

DECLARE_OP_INST2(group, LIST, NUMBER, ctx ATTR_UNUSED, vals)
{
    list_position pos = {&vals[0], 0};

    if (vals[1].v.num < 1.0)
        raise_error(ctx, APR_EINVAL);

    return generic_split(ctx, &pos, 
                         list_isend,
                         list_getprefix,
                         (size_t)vals[1].v.num);
}

DECLARE_OP_INST2(group, LIST, LIST, ctx ATTR_UNUSED, vals)
{
    list_position pos = {&vals[0], 0};
    return generic_tokenize(ctx, &pos, &vals[1],
                            list_find,
                            NULL,
                            list_getprefix);
}

static size_t list_find_trans(exec_context *ctx, const void *data, const void *sep)
{
    list_position pos = *(const list_position *)data;
    const closure *cls = sep;
    size_t count;

    if (pos.pos >= (unsigned)pos.list->v.list->nelts)
        return (size_t)(-1);

    for (count = 0; pos.pos + count < (unsigned)pos.list->v.list->nelts - 1;  count++)
    {
        expr_value vals[2];
        
        vals[0] = *ref_expr_list_value(ctx, *pos.list, pos.pos + count);
        vals[1] = *ref_expr_list_value(ctx, *pos.list, pos.pos + count + 1);
        if (call_predicate(ctx, cls, 2, vals))
            break;
    }
    return count;
}

DECLARE_OP_INST2(group, LIST, CLOSURE, ctx, vals)
{
    list_position pos = {&vals[0], 0};
    return generic_tokenize(ctx, &pos, &vals[1].v.cls,
                            list_find_trans,
                            NULL,
                            list_getprefix);
}

DECLARE_OP_INST2(eqv, CLOSURE, CLOSURE, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_eqv, 2);
    
    set_closure_arg(ctx, &clos, 0, vals[0]);
    set_closure_arg(ctx, &clos, 1, vals[1]);
    return clos;
}

DEFINE_OP_INFO(rem, normal, 2, NULL,
               DISP2(NUMBER, NUMBER, rem),
               DISP2(STRING, NUMBER, tokenize),
               DISP2(STRING, STRING, tokenize),
               DISP2(STRING, CLOSURE, tokenize),
               DISP1(BINARY, tokenize),
               DISP2(LIST, NUMBER, group),
               DISP2(LIST, LIST, group),
               DISP2(LIST, CLOSURE, group),
               DISP2(CLOSURE, CLOSURE, eqv));

ATTR_PURE
DECLARE_OP_INST2(power, NUMBER, NUMBER, ctx, vals)
{
    if (vals[1].v.num == 0.0)
        raise_error(ctx, APR_EINVAL);
    
    return MAKE_NUM_VALUE(pow(vals[0].v.num, vals[1].v.num));
}

DECLARE_OP_INST2(power, BINARY, BINARY, ctx, vals)
{
    if (!vals[0].v.bin.type->power)
        raise_error(ctx, APR_ENOTIMPL);
    return vals[0].v.bin.type->power(ctx, vals[0].v.bin, vals[1].v.bin);
}

DECLARE_OP_INST2(product, LIST, LIST, ctx, vals)
{
    unsigned i, j;
    expr_value result = make_expr_value_list(ctx, 
                                             (size_t)vals[0].v.list->nelts * 
                                             (size_t)vals[1].v.list->nelts);
    
    for (i = 0; i < (unsigned)vals[0].v.list->nelts; i++)
    {
        for (j = 0; j < (unsigned)vals[1].v.list->nelts; j++)
        {
            expr_value pair = make_expr_value_list(ctx, 2);
            set_expr_list_value(ctx, &pair, 0, 
                                *ref_expr_list_value(ctx, vals[0], i));
            set_expr_list_value(ctx, &pair, 1, 
                                *ref_expr_list_value(ctx, vals[1], j));
            set_expr_list_value(ctx, &result,
                                i * (unsigned)vals[1].v.list->nelts + j,
                                pair);
        }
    }
    return result;
}

DECLARE_OP_INST2(iter, CLOSURE, NUMBER, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_iter, 2);
    
    set_closure_arg(ctx, &clos, 0, vals[0]);
    set_closure_arg(ctx, &clos, 1, vals[1]);
    return clos;
}

DECLARE_OP_INST2(while, CLOSURE, CLOSURE, ctx, vals)
{
    expr_value clos = make_closure(ctx, &predicate_trf_while, 2);
    
    set_closure_arg(ctx, &clos, 0, vals[0]);
    set_closure_arg(ctx, &clos, 1, vals[1]);
    return clos;
}


DEFINE_OP_INFO(power, normal, 2, NULL,
               DISP2(NUMBER, NUMBER, power),
               DISP2(BINARY, BINARY, power),
               DISP2(LIST, LIST, product),
               DISP2(CLOSURE, NUMBER, iter),
               DISP2(CLOSURE, CLOSURE, while));

DEFINE_OP_INFO(join, normal, 2, eval_mkpair,
               DISP2(LIST, LIST, append),
               DISP1(LIST, append1));
