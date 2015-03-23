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

static ATTR_NONNULL ATTR_WARN_UNUSED_RESULT
expr_value do_typecast(exec_context *ctx, typecast_func cast, expr_value arg)
{
    arg = resolve_reference(arg, false);

    if (arg.type != VALUE_LIST_ITER)
        return cast(ctx, arg);
    else
    {
        expr_value result = make_expr_value_list(ctx, (size_t)arg.v.list->nelts);
        unsigned i;

        for (i = 0; i < (unsigned)arg.v.list->nelts; i++)
        {
            expr_value sub = do_typecast(ctx, cast, 
                                         *ref_expr_list_value(ctx, arg, i));
            set_expr_list_value(ctx, &result, i, sub);
        }
        return result;
    }
}

static ATTR_NONNULL ATTR_WARN_UNUSED_RESULT 
expr_value eval_at_context(exec_context *ctx, bool lvalue, expr_value new_this,
                           const expr_node *arg)
{
    new_this = resolve_reference(new_this, true);
    
    if (new_this.type != VALUE_LIST_ITER)
    {
        exec_context nested_ctx = *ctx;
        if (new_this.type != VALUE_NODEREF)
            raise_error(ctx, TEN_BAD_TYPE);
        nested_ctx.context_node = new_this.v.ref;
        return evaluate_expr_node(&nested_ctx, arg, lvalue);
    }
    else
    {
        expr_value result = make_expr_value_list(ctx, (size_t)new_this.v.list->nelts);
        unsigned i;

        for (i = 0; i < (unsigned)new_this.v.list->nelts; i++)
        {
            expr_value sub = eval_at_context(ctx, lvalue,
                                             *ref_expr_list_value(ctx, new_this, i),
                                             arg);
            set_expr_list_value(ctx, &result, i, sub);
        }
        return result;
    }
}                                 

static ATTR_NONNULL ATTR_WARN_UNUSED_RESULT
expr_value eval_defaulted(exec_context *ctx, bool lvalue, const expr_node *base, const expr_node *defval)
{
    volatile sigjmp_buf catcher;
    volatile exec_context gctx = *ctx;
    volatile expr_value result = NULL_VALUE;

    gctx.exitpoint = &catcher;
    if (!sigsetjmp(catcher, 1))
    {
        result = evaluate_expr_node(&gtx, base, lvalue);
    }
    if (result.type == VALUE_NULL ||
        (result.type == VALUE_NUMBER && isnan(result.v.num)))
    {
        result = evaluate_expr_node(ctx, defval, lvalue);
    }
    return result;
}

static ATTR_NONNULL ATTR_WARN_UNUSED_RESULT
expr_value evaluate_operator(exec_context *ctx, bool lvalue, 
                             const operator_info *op, expr_value *args)
{
    bool iterables = false;
    unsigned i;
    
    for (i = 0; i < op->n_args; i++)
    {
        args[i] = resolve_reference(args[i], op->info[i].lvalue);
        assert(!op->info[i].product);
        if (op->info[i].iterable && args[i].type == VALUE_LIST_ITER)
            iterables = true;
    }
    if (!iterables)
        return ((expr_operator_func)dispatch_values(ctx, op->func, op->n_args, args))
            (ctx, lvalue, op->n_args, args);
    else
    {
        expr_value result = make_expr_value_list(ctx, 0);
        expr_value new_args[3];
        unsigned offsets[3] = {0, 0, 0};

        memcpy(new_args, args, sizeof(new_args));

        for (;;)
        {
            for (i = 0; i < op->n_args; i++)
            {
                if (op->info[i].iterable && args[i].type == VALUE_LIST_ITER)
                {
                    if (offsets[i] == (unsigned)args[i].v.list->nelts)
                    {
                        iterables = false;
                        break;
                    }
                    new_args[i] = *ref_expr_list_value(ctx, args[i], offsets[i]);
                    offsets[i]++;
                }
            }
            if (!iterables)
                break;
            add_expr_list_value(ctx, &result,
                                evaluate_operator(ctx, lvalue, op, new_args));
        }
        return result;
    }
}

expr_value evaluate_expr_node(exec_context *ctx,
                              const expr_node *expr,
                              bool lvalue)
{
    switch (expr->type)
    {
        case EXPR_LITERAL:
            return expr->x.literal;
        case EXPR_VARREF:
        {
            tree_node *found = lookup_var(ctx, expr->x.varname, lvalue);
            if (found == NULL)
            {
                if (!lvalue)
                    raise_error(ctx, APR_ENOENT);
                else
                    found = define_var(ctx, expr->x.varname);
            }
            return MAKE_VALUE(NODEREF, found->scope, found);
        }
        case EXPR_LIST:
        {
            expr_value list = make_expr_value_list(ctx, 0);
            unsigned i;
            
            for (i = 0; i < (unsigned)expr->x.list->nelts; i++)
            {
                const expr_node *sub = APR_ARRAY_IDX(expr->x.list, i, const expr_node *);
                
                if (sub == NULL)
                {
                    add_expr_list_value(ctx, &list, NULL_VALUE);
                }
                else
                {
                    add_expr_list_value(ctx, &list, 
                                        evaluate_expr_node(ctx, sub, lvalue));
                }
            }
            return list;
        }
        case EXPR_CONTEXT:
            if (!ctx->context_node)
                return NULL_VALUE;
            else
                return MAKE_VALUE(NODEREF, ctx->local_pool, ctx->context_node);
        case EXPR_AT_CONTEXT:
            return eval_at_context(ctx, lvalue, 
                                   evaluate_expr_node(ctx, expr->x.at.context, false), 
                                   expr->x.at.arg);
        case EXPR_DEFAULT:
            return eval_defaulted(ctx, lvalue, expr->x.at.arg, expr->x.at.default);
        case EXPR_OPERATOR:
        {
            expr_value args[3];
            unsigned i;

            for (i = 0; i < expr->x.op.func->n_args; i++)
            {
                args[i] = evaluate_expr_node(ctx, expr->x.op.args[i], 
                                             lvalue && 
                                             expr->x.op.func->info[i].lvalue);
            }

            return evaluate_operator(ctx, lvalue, expr->x.op.func, args);
        }
        case EXPR_TYPECAST:
        {
            expr_value arg = evaluate_expr_node(ctx, expr->x.cast.arg, false);
            return do_typecast(ctx, expr->x.cast.func, arg);
        }
    }
    return NULL_VALUE;
}

expr_value typecast_tostring(exec_context *ctx, expr_value val)
{
    switch (val.type)
    {
        case VALUE_NULL:
            return MAKE_VALUE(STRING, ctx->static_pool, (const uint8_t *)"");
        case VALUE_NUMBER:
        {
            char buf[24];
            snprintf(buf, sizeof(buf), "%g", val.v.num);
            return MAKE_VALUE(STRING, ctx->local_pool,
                              (const uint8_t *)apr_pstrdup(ctx->local_pool, buf));
        }
        case VALUE_TIMESTAMP:
        {
            char buf[24];
            struct tm t;
            t = *localtime(&val.v.ts);
            strftime(buf, sizeof(buf), "%FT%T", &t);
            return MAKE_VALUE(STRING, ctx->local_pool,
                              (const uint8_t *)apr_pstrdup(ctx->local_pool, buf));
        }
        case VALUE_STRING:
            return val;
        case VALUE_BINARY:
            if (val.v.bin.type->to_string == NULL)
                raise_error(ctx, APR_ENOTIMPL);
            return MAKE_VALUE(STRING, ctx->local_pool, 
                              val.v.bin.type->to_string(ctx, val.v.bin));
        default:
            raise_error(ctx, TEN_BAD_TYPE);
    }
}

