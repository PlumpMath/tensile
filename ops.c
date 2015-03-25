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
    {false, true},
    {false, true},
    {false, true}
};

static const arg_info lvalue_args[3] = {
    {true, true},
    {false, true},
    {false, true}
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
        {VALUE_##_type1), VALUE_##_type2,}}


DECLARE_OP_INST(noop, ctx ATTR_UNUSED, vals)
{
    return vals[0];
}

#define eval_noop_NULL eval_noop

DEFINE_OP_INFO_GEN(noop, normal, 1, {0, eval_noop, {}});
        

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
               DISP1(NULL, noop),
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
               DISP1(NULL, noop),
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

DEFINE_OP_INST1(length, NULL, ctx ATTR_UNUSED, vals ATTR_UNUSED)
{
    return MAKE_NUM_VALUE(0.0);
}

DEFINE_OP_INST1(length, NUMBER, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE(log10(vals[0].v.num));
}

DEFINE_OP_INST1(length, STRING, ctx ATTR_UNUSED, vals)
{
    return MAKE_NUM_VALUE((double)u8_mbsnlen(val.v.str, 
                                             u8_strlen(val.v.str)));
}

/* static inline */
/* expr_value eval_length(exec_context *ctx, expr_value val) */
/* { */
/*     switch (val.type) */
/*     { */
/*         case VALUE_NULL: */
/*             return MAKE_NUM_VALUE(0.0); */
/*         case VALUE_NUMBER: */
/*             return MAKE_NUM_VALUE(log10(val.v.num)); */
/*         case VALUE_STRING: */
/*             return 
/*         case VALUE_LIST: */
/*             return MAKE_NUM_VALUE((double)val.v.list->nelts); */
/*         case VALUE_BINARY: */
/*             if (val.v.bin.type->length == NULL) */
/*                 raise_error(ctx, APR_ENOTIMPL); */
/*             return MAKE_NUM_VALUE((double)val.v.bin.type->length(ctx, val.v.bin)); */
/*         default: */
/*             raise_error(ctx, TEN_BAD_TYPE); */
/*     } */
/* } */

/* DEFINE_NORMAL_UNOP(length, eval_length); */


/* static inline */
/* expr_value eval_loosen(exec_context *ctx, expr_value val) */
/* { */
/*     switch (val.type) */
/*     { */
/*         case VALUE_NULL: */
/*             return NULL_VALUE; */
/*         case VALUE_NUMBER: */
/*         { */
/*             return MAKE_NUM_VALUE(round(val.v.num)); */
/*         } */
/*         case VALUE_STRING: */
/*         { */
/*             const char *lang = uc_locale_language(); */
/*             size_t destlen = 0; */
/*             char *xfrm = u8_casexfrm(val.v.str, u8_strlen(val.v.str), lang, */
/*                                      UNINORM_NFKC, NULL, &destlen); */
/*             uint8_t *dest; */
            
/*             if (xfrm == NULL) */
/*                 raise_error(ctx, APR_FROM_OS_ERROR(errno)); */
/*             dest = apr_palloc(ctx->local_pool, destlen); */
/*             memcpy(dest, xfrm, destlen); */
/*             dest[destlen] = '\0'; */
/*             val.v.str = dest; */
/*             val.scope = ctx->local_pool; */
/*             free(xfrm); */
/*             return val; */
/*         } */
/*         case VALUE_TIMESTAMP: */
/*             return MAKE_VALUE(TIMESTAMP, val.scope, */
/*                               timestamp_just_date(val.v.ts)); */
/*         case VALUE_BINARY: */
/*             if (val.v.bin.type->loosen == NULL) */
/*                 raise_error(ctx, APR_ENOTIMPL); */
/*             return val.v.bin.type->loosen(ctx, val.v.bin); */
/*         case VALUE_CLOSURE: */
/*         default: */
/*             raise_error(ctx, TEN_BAD_TYPE); */
/*     } */
/* } */


/* DEFINE_NORMAL_UNOP(loosen, eval_loosen); */

/* enum top_char_class { */
/*     CC_NONE, */
/*     CC_ALPHA, */
/*     CC_NUMBER, */
/*     CC_PUNCT, */
/*     CC_SYMBOL, */
/*     CC_SEPARATOR, */
/*     CC_OTHER */
/* }; */

/* static inline enum top_char_class get_top_char_class(ucs4_t ch) */
/* { */
/*     if (uc_is_general_category(ch, UC_LETTER) || */
/*         uc_is_general_category(ch, UC_MARK)) */
/*         return CC_ALPHA; */
/*     if (uc_is_general_category(ch, UC_NUMBER)) */
/*         return CC_NUMBER; */
/*     if (uc_is_general_category(ch, UC_PUNCTUATION)) */
/*         return CC_PUNCT; */
/*     if (uc_is_general_category(ch, UC_SYMBOL)) */
/*         return CC_SYMBOL; */
/*     if (uc_is_general_category(ch, UC_SEPARATOR)) */
/*         return CC_SEPARATOR; */
/*     return CC_OTHER; */
/* } */

/* static inline */
/* expr_value eval_frac(exec_context *ctx, expr_value val) */
/* { */
/*     switch (val.type) */
/*     { */
/*         case VALUE_NULL: */
/*             return NULL_VALUE; */
/*         case VALUE_NUMBER: */
/*         { */
/*             double ipart; */
            
/*             return MAKE_NUM_VALUE(modf(val.v.num, &ipart)); */
/*         } */
/*         case VALUE_STRING: */
/*         { */
/*             const uint8_t *iter = val.v.str; */
/*             const uint8_t *start = iter; */
/*             enum top_char_class prev = CC_NONE; */
/*             apr_array_header_t *list = apr_array_make(ctx->local_pool, 0, sizeof(expr_value)); */
/*             uint8_t *token; */

/*             for (;;) { */
/*                 ucs4_t ch; */
/*                 enum top_char_class cur; */
                
/*                 iter = u8_next(&ch, iter); */
/*                 if (iter == NULL) */
/*                     break; */
/*                 cur = get_top_char_class(ch); */
/*                 if (cur != prev) */
/*                 { */
/*                     if (start != iter) */
/*                     { */
/*                         token = (uint8_t *)apr_pstrndup(ctx->local_pool, (const char *)start, (apr_size_t)(iter - start)); */
/*                         APR_ARRAY_PUSH(list, expr_value) = MAKE_VALUE(STRING, ctx->local_pool, token); */
/*                         start = iter; */
/*                     } */
/*                     prev = cur; */
/*                 } */
/*             } */
/*             if (*start) */
/*             { */
/*                 token = (uint8_t *)apr_pstrdup(ctx->local_pool, (const char *)start); */
/*                 APR_ARRAY_PUSH(list, expr_value) = MAKE_VALUE(STRING, ctx->local_pool, token); */
/*             } */
/*         } */
/*         case VALUE_BINARY: */
/*             if (val.v.bin.type->split == NULL) */
/*                 raise_error(ctx, APR_ENOTIMPL); */
/*             return val.v.bin.type->split(ctx, val.v.bin, NULL); */
/*         default: */
/*             raise_error(ctx, TEN_BAD_TYPE); */
/*     } */
/* } */


/* DEFINE_NORMAL_UNOP(frac, eval_frac); */


/* static inline */
/* expr_value eval_floor(exec_context *ctx, expr_value val) */
/* { */
/*     switch (val.type) */
/*     { */
/*         case VALUE_NULL: */
/*             return NULL_VALUE; */
/*         case VALUE_NUMBER: */
/*         { */
/*             double ipart; */
/*             modf(val.v.num, &ipart); */
/*             return MAKE_NUM_VALUE(ipart); */
/*         } */
/*         case VALUE_STRING: */
/*         { */
/*             size_t len = u8_strlen(val.v.str); */
/*             char *breaks = apr_palloc(ctx->local_pool, len + 1); */
/*             unsigned i; */
/*             unsigned wordstart = 0; */
/*             apr_array_header_t *list = apr_array_make(ctx->local_pool, 0, */
/*                                                       sizeof(expr_value)); */
            
/*             breaks[len] = 1; */
/*             u8_wordbreaks(val.v.str, len, breaks); */

/*             for (i = 0; i <= len; i++) */
/*             { */
/*                 if (breaks[i]) */
/*                 { */
/*                     APR_ARRAY_PUSH(list, expr_value) = */
/*                                    MAKE_VALUE(STRING, ctx->local_pool, */
/*                                               (const uint8_t *) */
/*                                               apr_pstrndup(ctx->local_pool, */
/*                                                            (const char *) */
/*                                                            (val.v.str + wordstart), */
/*                                                            i - wordstart)); */
/*                     wordstart = i; */
/*                 } */
/*             } */
/*             return MAKE_VALUE(LIST, ctx->local_pool, list); */
/*         } */
/*         case VALUE_BINARY: */
/*             if (val.v.bin.type->tokenize == NULL) */
/*                 raise_error(ctx, APR_ENOTIMPL); */
/*             return val.v.bin.type->tokenize(ctx, val.v.bin, NULL); */
/*         default: */
/*             raise_error(ctx, TEN_BAD_TYPE); */
/*     } */
/* } */


/* DEFINE_NORMAL_UNOP(floor, eval_floor); */


/* static inline ATTR_PURE */
/* expr_value eval_parent(exec_context *ctx, expr_value val) */
/* { */
/*     switch (val.type) */
/*     { */
/*         case VALUE_NULL: */
/*             return NULL_VALUE; */
/*         case VALUE_NODEREF: */
/*             if (!val.v.ref->parent) */
/*                 return NULL_VALUE; */
            
/*             return MAKE_VALUE(NODEREF, */
/*                               val.v.ref->parent->scope, */
/*                               val.v.ref->parent); */
/*         default: */
/*             raise_error(ctx, TEN_BAD_TYPE); */
/*     } */
/* } */

/* DEFINE_REF_UNOP(parent, eval_parent); */


/* static inline */
/* expr_value eval_count(exec_context *ctx, expr_value val) */
/* { */
/*     switch (val.type) */
/*     { */
/*         case VALUE_NULL: */
/*             return MAKE_NUM_VALUE(0.0); */
/*         case VALUE_NODEREF: */
/*             if (!val.v.ref->attrs) */
/*                 return MAKE_NUM_VALUE(0.0); */
/*             return MAKE_NUM_VALUE((double)apr_hash_count(val.v.ref->attrs)); */
/*         default: */
/*             raise_error(ctx, TEN_BAD_TYPE); */
/*     } */
/* } */

/* DEFINE_REF_UNOP(count, eval_count); */

/* static inline */
/* expr_value eval_plus(exec_context *ctx, expr_value val1, expr_value val2, */
/*                      bool lvalue ATTR_UNUSED) */
/* { */
/*     switch (val1.type) */
/*     { */
/*         case VALUE_NUMBER: */
/*             if (val2.type != VALUE_NUMBER) */
/*                 break; */
            
/*             return MAKE_NUM_VALUE(val1.v.num + val2.v.num); */
/*         case VALUE_STRING: */
/*             if (val2.type == VALUE_NULL) */
/*                 return val1; */

/*             if (val2.type != VALUE_STRING) */
/*                 break; */
            
/*             return MAKE_VALUE(STRING, ctx->local_pool, */
/*                               (const uint8_t *) */
/*                               apr_pstrcat(ctx->local_pool, */
/*                                           val1.v.str, val2.v.str, NULL)); */
/*         case VALUE_TIMESTAMP: */
/*             if (val2.type != VALUE_NUMBER) */
/*                 break; */
            
/*             return MAKE_VALUE(TIMESTAMP, val1.scope, */
/*                               val1.v.ts + (time_t)val2.v.num); */
/*         case VALUE_BINARY: */
/*             if (val2.type != VALUE_BINARY) */
/*                 break; */
            
/*             if (!val1.v.bin.type->concat) */
/*                 raise_error(ctx, APR_ENOTIMPL); */
/*             return val1.v.bin.type->concat(ctx, val1.v.bin, val2.v.bin); */
/*         case VALUE_LIST: */
/*         { */
/*             expr_value result; */
/*             unsigned i; */

/*             if (val2.type == VALUE_NULL) */
/*                 return val1; */

/*             if (val2.type != VALUE_LIST) */
/*                 break; */
/*             result = make_expr_value_list(ctx, (size_t)val1.v.list->nelts + */
/*                                           (size_t)val2.v.list->nelts); */
/*             for (i = 0; i < (unsigned)val1.v.list->nelts; i++) */
/*             { */
/*                 set_expr_list_value(ctx, &result, i, */
/*                                     *ref_expr_list_value(ctx, val1, i)); */
/*             } */
/*             for (i = 0; i < (unsigned)val2.v.list->nelts; i++) */
/*             { */
/*                 set_expr_list_value(ctx, &result, */
/*                                     i + (unsigned)val1.v.list->nelts, */
/*                                     *ref_expr_list_value(ctx, val2, i)); */
/*             } */
/*             return result; */
/*         } */
/*         case VALUE_CLOSURE: */
/*         { */
/*             expr_value clos; */
/*             if (val2.type != VALUE_CLOSURE) */
/*                 break; */

/*             clos = make_closure(ctx, &predicate_trf_alt, 2); */

/*             set_closure_arg(ctx, &clos, 0, val1); */
/*             set_closure_arg(ctx, &clos, 1, val2); */
/*             return clos; */
/*         } */
/*         default: */
/*             /\* do nothing *\/ */
/*             break; */
/*     } */
/*     raise_error(ctx, TEN_BAD_TYPE); */
/* } */

/* DEFINE_NORMAL_BINOP(plus, eval_plus); */

/* static ATTR_NONNULL ATTR_WARN_UNUSED_RESULT */
/* bool is_list_suffix(exec_context *ctx, apr_array_header_t *list, */
/*                     apr_array_header_t *suffix) */
/* { */
/*     unsigned i; */

/*     if (suffix->nelts > list->nelts) */
/*         return false; */

/*     for (i = 0; i < (unsigned)suffix->nelts; i++) */
/*     { */
/*         if (compare_values(ctx, false, */
/*                            APR_ARRAY_IDX(suffix, i, expr_value), */
/*                            APR_ARRAY_IDX(list, */
/*                                          (int)i + (list->nelts - suffix->nelts), */
/*                                          expr_value), */
/*                            false) != 0) */
/*             return false; */
/*     } */
/*     return true; */
/* } */

/* static inline */
/* expr_value eval_minus(exec_context *ctx, expr_value val1, expr_value val2, */
/*                       bool lvalue ATTR_UNUSED) */
/* { */
/*     switch (val1.type) */
/*     { */
/*         case VALUE_NUMBER: */
/*             if (val2.type != VALUE_NUMBER) */
/*                 break; */
            
/*             return MAKE_NUM_VALUE(val1.v.num - val2.v.num); */
/*         case VALUE_STRING: */
/*             if (val2.type == VALUE_NULL) */
/*                 return val1; */
            
/*             if (val2.type != VALUE_STRING) */
/*                 break; */
            
/*             if (!u8_endswith(val1.v.str, val2.v.str)) */
/*                 return val1; */
            
/*             return MAKE_VALUE(STRING, ctx->local_pool, */
/*                               (const uint8_t *) */
/*                               apr_pstrndup(ctx->local_pool, */
/*                                            (const char *)val1.v.str, */
/*                                            u8_strlen(val1.v.str) - */
/*                                            u8_strlen(val2.v.str))); */
/*         case VALUE_TIMESTAMP: */
/*             if (val2.type == VALUE_NUMBER) */
/*                 return MAKE_VALUE(TIMESTAMP, val1.scope, */
/*                                   val1.v.ts - (time_t)val2.v.num); */
/*             if (val2.type == VALUE_TIMESTAMP) */
/*                 return MAKE_NUM_VALUE(difftime(val1.v.ts, val2.v.ts)); */
/*             break; */
/*         case VALUE_BINARY: */
/*             if (val2.type != VALUE_BINARY) */
/*                 break; */
            
/*             if (!val1.v.bin.type->strip) */
/*                 raise_error(ctx, APR_ENOTIMPL); */
/*             return val1.v.bin.type->strip(ctx, val1.v.bin, val2.v.bin); */
/*         case VALUE_LIST: */
/*         { */
/*             expr_value result; */
/*             unsigned i; */

/*             if (val2.type == VALUE_NULL) */
/*                 return val1; */

/*             if (val2.type != VALUE_LIST) */
/*                 break; */

/*             if (!is_list_suffix(ctx, val1.v.list, val2.v.list)) */
/*                 return val1; */
            
/*             result = make_expr_value_list(ctx, (size_t)val1.v.list->nelts - */
/*                                           (size_t)val1.v.list->nelts); */
/*             for (i = 0; i < (unsigned)result.v.list->nelts; i++) */
/*             { */
/*                 set_expr_list_value(ctx, &result, i, */
/*                                     *ref_expr_list_value(ctx, val1, i)); */
/*             } */
/*             return result; */
/*         } */
/*         case VALUE_CLOSURE: */
/*         { */
/*             expr_value clos; */
/*             if (val2.type != VALUE_CLOSURE) */
/*                 break; */

/*             clos = make_closure(ctx, &predicate_trf_alt, 2); */

/*             set_closure_arg(ctx, &clos, 0, val1); */
/*             set_closure_arg(ctx, &clos, 1, val2); */
/*             return clos; */
/*         } */
/*         default: */
/*             /\* do nothing *\/ */
/*             break; */
/*     } */
/*     raise_error(ctx, TEN_BAD_TYPE); */
/* } */

/* DEFINE_NORMAL_BINOP(minus, eval_minus); */


/* static inline */
/* expr_value eval_mul(exec_context *ctx, expr_value val1, expr_value val2, */
/*                       bool lvalue ATTR_UNUSED) */
/* { */
/*     switch (val1.type) */
/*     { */
/*         case VALUE_NUMBER: */
/*             if (val2.type != VALUE_NUMBER) */
/*                 break; */
            
/*             return MAKE_NUM_VALUE(val1.v.num * val2.v.num); */
/*         case VALUE_STRING: */
/*         { */
/*             size_t factor; */
/*             size_t srclen; */
/*             uint8_t *buf; */
/*             unsigned i; */

/*             if (val2.type != VALUE_NUMBER) */
/*                 break; */
/*             if (val2.v.num < 1.0) */
/*                 return MAKE_VALUE(STRING, ctx->static_pool, (const uint8_t *)""); */
/*             if (val2.v.num == 1.0) */
/*                 return val1; */

/*             factor = (size_t)val2.v.num; */
/*             srclen = u8_strlen(val1.v.str); */
/*             buf = apr_palloc(ctx->local_pool, factor * srclen + 1); */
/*             buf[factor * srclen] = '\0'; */

/*             for (i = 0; i < factor; i++) */
/*             { */
/*                 memcpy(buf + i * srclen, val1.v.str, srclen); */
/*             } */
/*             return MAKE_VALUE(STRING, ctx->local_pool, buf); */
/*         } */
/*         case VALUE_BINARY: */
/*             if (val2.type != VALUE_BINARY) */
/*                 break; */
            
/*             if (!val1.v.bin.type->intersperse) */
/*                 raise_error(ctx, APR_ENOTIMPL); */
/*             return val1.v.bin.type->intersperse(ctx, val1.v.bin, val2.v.bin); */
/*         case VALUE_LIST: */
/*         { */
/*             expr_value result; */
/*             unsigned i; */

/*             if (val2.type == VALUE_NULL) */
/*                 return val1; */

/*             if (val2.type != VALUE_LIST) */
/*                 break; */

/*             if (!is_list_suffix(ctx, val1.v.list, val2.v.list)) */
/*                 return val1; */
            
/*             result = make_expr_value_list(ctx, (size_t)val1.v.list->nelts - */
/*                                           (size_t)val1.v.list->nelts); */
/*             for (i = 0; i < (unsigned)result.v.list->nelts; i++) */
/*             { */
/*                 set_expr_list_value(ctx, &result, i, */
/*                                     *ref_expr_list_value(ctx, val1, i)); */
/*             } */
/*             return result; */
/*         } */
/*         case VALUE_CLOSURE: */
/*         { */
/*             expr_value clos; */
/*             if (val2.type != VALUE_CLOSURE) */
/*                 break; */

/*             clos = make_closure(ctx, &predicate_trf_and, 2); */

/*             set_closure_arg(ctx, &clos, 0, val1); */
/*             set_closure_arg(ctx, &clos, 1, val2); */
/*             return clos; */
/*         } */
/*         default: */
/*             /\* do nothing *\/ */
/*             break; */
/*     } */
/*     raise_error(ctx, TEN_BAD_TYPE); */
/* } */

/* DEFINE_NORMAL_BINOP(mul, eval_mul); */
