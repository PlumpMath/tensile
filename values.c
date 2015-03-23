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

#include <unicase.h>
#include "engine.h"
#include "values.h"

static void ensure_expr_list_accessible(exec_context *ctx, apr_pool_t *destpool, 
                                        apr_array_header_t **hdr)
{
    unsigned i;
    
    if (!*hdr)
        return;
    if (!apr_pool_is_ancestor((*hdr)->pool, destpool))
        *hdr = apr_array_copy(destpool, *hdr);
    for (i = 0; i < (unsigned)(*hdr)->nelts; i++)
    {
        ensure_expr_value_accessible(ctx, destpool, 
                                     &APR_ARRAY_IDX(*hdr, i, expr_value));
    }
}

void ensure_expr_value_accessible(exec_context *ctx, apr_pool_t *destpool, 
                                  expr_value *val)
{
    switch (val->type)
    {
        case VALUE_STRING:
            if (apr_pool_is_ancestor(val->scope, destpool))
                return;
            val->v.str = (const uint8_t *)apr_pstrdup(destpool, (const char *)val->v.str);
            val->scope = destpool;
            break;
        case VALUE_BINARY:
            if (apr_pool_is_ancestor(val->scope, destpool))
                return;
            val->v.bin.data = apr_pmemdup(destpool, val->v.bin.data,
                                          val->v.bin.len);
            if (val->v.bin.type->aftercopy)
                val->v.bin.type->aftercopy(ctx, val);
            val->scope = destpool;
            break;
        case VALUE_NODEREF:
            if (apr_pool_is_ancestor(val->v.ref->scope, destpool))
                return;
            raise_error(ctx, TEN_DANGLING_REF);
            break;
        case VALUE_LIST:
        case VALUE_LIST_ITER:
            ensure_expr_list_accessible(ctx, destpool, &val->v.list);
            break;
        case VALUE_CLOSURE:
            ensure_expr_list_accessible(ctx, destpool, &val->v.cls.args);
            break;
        default:
            /* do nothing */
            break;
    }
}

static inline ATTR_NONNULL ATTR_WARN_UNUSED_RESULT ATTR_MALLOC 
apr_array_header_t *new_list_of_values(exec_context *ctx, size_t n)
{
    apr_array_header_t *list = apr_array_make(ctx->local_pool, (int)n, sizeof(expr_value));
    unsigned i;
    
    for (i = 0; i < n; i++)
        APR_ARRAY_IDX(list, i, expr_value) = INIT_SCOPED_VALUE(ctx->local_pool);
    return list;
}

expr_value make_expr_value_list(exec_context *ctx, size_t n)
{
    return MAKE_VALUE(LIST, ctx->local_pool, 
                      new_list_of_values(ctx, n));
}

void add_expr_list_value(exec_context *ctx, expr_value *dest, expr_value src)
{
    expr_value *item;

    assert(dest->type == VALUE_LIST);
    item = &APR_ARRAY_PUSH(dest->v.list, expr_value);
    *item = INIT_SCOPED_VALUE(dest->scope);
    set_expr_value(ctx, item, src);
}

void set_expr_list_value(exec_context *ctx, expr_value *dest, 
                         unsigned idx, expr_value src)
{
    expr_value *item;

    assert(dest->type == VALUE_LIST);
    assert(idx < (unsigned)dest->v.list->nelts);
    
    item = &APR_ARRAY_IDX(dest->v.list, idx, expr_value);
    *item = INIT_SCOPED_VALUE(dest->scope);
    set_expr_value(ctx, item, src);
}

expr_value make_closure(struct exec_context *ctx, const struct action_def *action,
                        unsigned reserved)
{
    closure cls = {action, NULL};
    
    if (reserved > 0)
    {
        cls.args = new_list_of_values(ctx, reserved);
    }
    return MAKE_VALUE(CLOSURE, ctx->local_pool, cls);
}

void set_closure_arg(struct exec_context *ctx, expr_value *dest, 
                     unsigned narg, expr_value src)
{
    expr_value *item;

    assert(dest->type == VALUE_CLOSURE);
    assert(dest->v.cls.args != NULL);
    assert(narg < (unsigned)dest->v.cls.args->nelts);

    item = &APR_ARRAY_IDX(dest->v.cls.args, narg, expr_value);
    *item = INIT_SCOPED_VALUE(dest->scope);
    set_expr_value(ctx, item, src);
}


tree_node *new_tree_node(apr_pool_t *scope)
{
    tree_node *n = apr_palloc(scope, sizeof(*n));

    n->scope = scope;
    n->value = INIT_SCOPED_VALUE(scope);
    n->attrs = NULL;
    n->parent = NULL;
    n->notifier = NULL;
    n->local = false;
    return n;
}

static void ATTR_NONNULL
subtree_change(exec_context *ctx, tree_node *node)
{
    tree_node *iter;
    for (iter = node->parent; iter != NULL; iter = iter->parent)
    {
        if (iter->notifier)
            iter->notifier(ctx, TREE_NOTIFY_SUBTREE_CHANGE, iter, node);
    }
}

tree_node *add_tree_node_attr(exec_context *ctx, tree_node *node, const uint8_t *key, 
                              bool replace)
{
    tree_node *child;
    if (node->notifier)
    {
        if (!node->notifier(ctx, TREE_NOTIFY_BEFORE_ADD_CHILD, node, key))
            raise_error(ctx, APR_EACCES);
    }
    if (!node->attrs)
        node->attrs = apr_hash_make(node->scope);
    
    child = apr_hash_get(node->attrs, key, APR_HASH_KEY_STRING);
        
    if (child != NULL)
    {
        if (!replace)
            raise_error(ctx, APR_EEXIST);
    }
    else
    {
        child = new_tree_node(node->scope);
        child->parent = node;
        apr_hash_set(node->attrs, apr_pstrdup(node->scope, (const char *)key), 
                     APR_HASH_KEY_STRING, child);
    }
    
    if (node->notifier)
        node->notifier(ctx, TREE_NOTIFY_AFTER_ADD_CHILD, node, key, child);
    subtree_change(ctx, node);
    return child;
}

void delete_tree_node_attr(exec_context *ctx, tree_node *node, const uint8_t *key)
{
    if (!node->attrs)
        return;
    
    if (node->notifier)
    {
        if (!node->notifier(ctx, TREE_NOTIFY_BEFORE_DELETE_CHILD, node, key))
            raise_error(ctx, APR_EACCES);
    }
    if (key == NULL)
        apr_hash_clear(node->attrs);
    else
        apr_hash_set(node->attrs, key, APR_HASH_KEY_STRING, NULL);
    if (node->notifier)
    {
        node->notifier(ctx, TREE_NOTIFY_AFTER_DELETE_CHILD, node, key);
    }
    subtree_change(ctx, node);
}

tree_node *lookup_tree_node_attr(exec_context *ctx ATTR_UNUSED, tree_node *node, const uint8_t *key)
{
    if (!node->attrs)
        return NULL;
    
    return apr_hash_get(node->attrs, key, APR_HASH_KEY_STRING);
}

void update_tree_node(exec_context *ctx, tree_node *node)
{
    if (node->notifier)
    {
        if (!node->notifier(ctx, TREE_NOTIFY_UPDATE_VALUE, node))
            raise_error(ctx, APR_EACCES);
    }
}

void set_tree_node_value(exec_context *ctx, tree_node *dest, expr_value src)
{
    expr_value old = dest->value;
    if (dest->notifier)
    {
        if (!dest->notifier(ctx, TREE_NOTIFY_CHANGE_VALUE, dest, src))
            raise_error(ctx, APR_EACCES);
    }
    set_expr_value(ctx, &old, src);
    dest->value = old;
}

void add_tree_node_list_value(exec_context *ctx, tree_node *dest, expr_value src)
{
    expr_value old = dest->value;

    assert(dest->value.type == VALUE_LIST);
    old.v.list = apr_array_copy(dest->scope, old.v.list);
    add_expr_list_value(ctx, &old, src);
    if (dest->notifier)
    {
        if (!dest->notifier(ctx, TREE_NOTIFY_CHANGE_VALUE, dest, old))
            raise_error(ctx, APR_EACCES);
    }
    dest->value = old;
}

void set_tree_node_list_value(exec_context *ctx, tree_node *dest, 
                              unsigned idx, expr_value src)
{
    expr_value old = dest->value;

    assert(dest->value.type == VALUE_LIST);
    old.v.list = apr_array_copy(dest->scope, old.v.list);
    set_expr_list_value(ctx, &old, idx, src);
    if (dest->notifier)
    {
        if (!dest->notifier(ctx, TREE_NOTIFY_CHANGE_VALUE, dest, old))
            raise_error(ctx, APR_EACCES);
    }
    dest->value = old;
}

void copy_tree_node(struct exec_context *ctx, tree_node *dest, tree_node *src)
{
    update_tree_node(ctx, src);
    delete_tree_node_attr(ctx, dest, NULL);
    set_tree_node_value(ctx, dest, src->value);

    if (src->attrs)
    {
        apr_hash_index_t *iter;
        for (iter = apr_hash_first(ctx->local_pool, src->attrs);
             iter != NULL;
             iter = apr_hash_next(iter))
        {
            const uint8_t *attr;
            tree_node *srcchild;
            tree_node *destchild;
            
            apr_hash_this(iter, (const void **)&attr, NULL, (void **)&srcchild);
            destchild = add_tree_node_attr(ctx, dest, attr, false);
            copy_tree_node(ctx, destchild, srcchild);
        }
    }
}

expr_value resolve_reference(expr_value val, bool lvalue)
{
    expr_value *iter = &val;
    if (val.type != VALUE_NODEREF)
        return val;
    
    while ((lvalue ? iter->v.ref->value.type : iter->type) == VALUE_NODEREF)
        iter = &iter->v.ref->value;

    return *iter;
}

static int compare_lists(exec_context *ctx, bool inexact, const apr_array_header_t *l1, 
                         const apr_array_header_t *l2, bool ordering)
{
    if (!l1)
        return l2 == NULL ? 0 : -1;
    if (!l2)
        return 1;
    else
    {
        unsigned common = (unsigned)(l1->nelts > l2->nelts ? 
                                     l2->nelts : 
                                     l1->nelts);
        unsigned i;
        int result;
        
        for (i = 0; i < common; i++)
        {
            result = compare_values(ctx, inexact,
                                    APR_ARRAY_IDX(l1, i, expr_value),
                                    APR_ARRAY_IDX(l2, i, expr_value),
                                    ordering);
            if (result != 0)
                return result;
        }
        return l1->nelts - l2->nelts;
    }
}

int compare_values(exec_context *ctx, bool inexact, expr_value v1, expr_value v2, bool ordering)
{
    if (v1.type != v2.type)
        return (int)v1.type - (int)v2.type;
    
    switch (v1.type)
    {
        case VALUE_NULL:
            return 0;
        case VALUE_NUMBER:
            if (inexact)
            {
                v1.v.num = round(v1.v.num);
                v2.v.num = round(v2.v.num);
            }
            return v1.v.num > v2.v.num ? 1 : v1.v.num < v2.v.num ? -1 : 0;
        case VALUE_TIMESTAMP:
            if (inexact)
            {
                v1.v.ts = timestamp_just_date(v1.v.ts);
                v2.v.ts = timestamp_just_date(v2.v.ts);
            }
            return v1.v.ts > v2.v.ts ? 1 : v1.v.ts < v2.v.ts ? -1 : 0;
        case VALUE_STRING:
            if (inexact)
            {
                const char *lang = uc_locale_language();
                int result;

                if (u8_casecoll(v1.v.str, u8_strlen(v1.v.str),
                                v2.v.str, u8_strlen(v2.v.str),
                                lang,
                                UNINORM_NFKC,
                                &result))
                {
                    raise_error(ctx, APR_FROM_OS_ERROR(errno));
                }
                return result;
            }
            return u8_strcmp(v1.v.str, v2.v.str);
        case VALUE_BINARY:
            if (!v1.v.bin.type->compare)
                raise_error(ctx, APR_ENOTIMPL);
            return v1.v.bin.type->compare(ctx, inexact, v1.v.bin, v2.v.bin, ordering);
        case VALUE_LIST:
        case VALUE_LIST_ITER:
        {
            return compare_lists(ctx, inexact, v1.v.list, v2.v.list, ordering);
        }
        case VALUE_CLOSURE:
        {
            if (ordering)
                raise_error(ctx, APR_ENOTIMPL);

            if (inexact)
            {
                return v1.v.cls.def != v2.v.cls.def;
            }
            if (v1.v.cls.def != v2.v.cls.def)
                return 1;
            return compare_lists(ctx, inexact, v1.v.cls.args, v2.v.cls.args, ordering);
        }
        case VALUE_NODEREF:
        {
            if (ordering)
                raise_error(ctx, APR_ENOTIMPL);
            
            return v1.v.ref != v2.v.ref;
        }
        default:
            assert(0);
    }
}

void *dispatch_values(struct exec_context *ctx, const dispatch *disp, 
                      unsigned nargs, const expr_value *vals)
{
    unsigned i;
    
    for (i = 0; i < nargs; i++)
    {
        if (disp->types[vals[i].type].leaf)
        {
            void *result = disp->types[vals[i].type].x.func;

            if (result == NULL)
                raise_error(ctx, TEN_BAD_TYPE);
            
            return result;
        }
        else
        {
            disp = disp->types[vals[i].type].x.next;
            if (disp == NULL)
                raise_error(ctx, TEN_BAD_TYPE);
        }
    }
    raise_error(ctx, TEN_TOO_FEW_ARGS);
}
