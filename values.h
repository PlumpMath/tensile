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
 * @brief low-level value definitions
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef VALUES_H
#define VALUES_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <apr_hash.h>
#include <apr_tables.h>
#include "support.h"

struct exec_context;

enum value_type {
    VALUE_NULL,
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_BINARY,
    VALUE_TIMESTAMP,
    VALUE_NODEREF,
    VALUE_LIST,
    VALUE_LIST_ITER,
    VALUE_CLOSURE
};
#define VALUE_MAX_TYPE VALUE_CLOSURE

typedef struct binary_data {
    const struct binary_data_type *type;
    size_t len;
    uint8_t *data;
} binary_data;

struct action_def;
typedef struct closure {
    const struct action_def *def;
    apr_array_header_t *args;
} closure;

typedef struct expr_value {
    enum value_type type;
    apr_pool_t *scope;
    union {
        double num;
        const uint8_t *str;
        time_t ts;
        binary_data bin;
        struct tree_node *ref;
        apr_array_header_t *list;
        closure cls;
    } v;
} expr_value;

#define VALUE_FIELD_NUMBER num
#define VALUE_FIELD_STRING str
#define VALUE_FIELD_BINARY bin
#define VALUE_FIELD_TIMESTAMP ts
#define VALUE_FIELD_NODEREF ref
#define VALUE_FIELD_LIST list
#define VALUE_FIELD_LIST_ITER list
#define VALUE_FIELD_CLOSURE cls

#define MAKE_VALUE(_type, _scope, _value)                           \
    ((expr_value){.type = VALUE_##_type,                            \
            .scope = (_scope),                                      \
            .v = {.VALUE_FIELD_##_type = (_value)}})

#define MAKE_NUM_VALUE(_value) MAKE_VALUE(NUMBER, NULL, _value)


#define INIT_SCOPED_VALUE(_scope)                           \
    ((expr_value){.type = VALUE_NULL, .scope = (_scope)})

#define NULL_VALUE INIT_SCOPED_VALUE(NULL)

extern expr_value make_expr_value_list(struct exec_context *ctx, size_t n)
    ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT;

static inline ATTR_NONNULL_1ST
void set_expr_value_num(expr_value *dest, double val)
{
    dest->type = VALUE_NUMBER;
    dest->v.num = val;
}

extern void ensure_expr_value_accessible(struct exec_context *ctx, apr_pool_t *destpool, 
                                         expr_value *val)
    ATTR_NONNULL;

static inline ATTR_NONNULL
void set_expr_value(struct exec_context *ctx, expr_value *dest, expr_value src)
{
    ensure_expr_value_accessible(ctx, dest->scope, &src);
    *dest = src;
}

extern expr_value make_expr_value_list(struct exec_context *ctx, size_t n)
    ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT;

static inline ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT ATTR_PURE
expr_value *ref_expr_list_value(struct exec_context *ctx ATTR_UNUSED, expr_value src, unsigned idx)
{
    assert(src.type == VALUE_LIST || src.type == VALUE_LIST_ITER);
    assert(idx < (unsigned)src.v.list->nelts);
    
    return &APR_ARRAY_IDX(src.v.list, idx, expr_value);
}

extern void add_expr_list_value(struct exec_context *ctx, expr_value *dest, expr_value src)
    ATTR_NONNULL;

extern void set_expr_list_value(struct exec_context *ctx, expr_value *dest, 
                                unsigned idx, expr_value src)
    ATTR_NONNULL;

extern expr_value make_closure(struct exec_context *ctx, const struct action_def *action,
                               unsigned reserved)
    ATTR_NONNULL ATTR_WARN_UNUSED_RESULT;

extern void set_closure_arg(struct exec_context *ctx, expr_value *dest, 
                            unsigned narg, expr_value src)
    ATTR_NONNULL;


enum tree_notify_type {
    TREE_NOTIFY_UPDATE_VALUE,
    TREE_NOTIFY_CHANGE_VALUE,
    TREE_NOTIFY_BEFORE_ADD_CHILD,
    TREE_NOTIFY_AFTER_ADD_CHILD,
    TREE_NOTIFY_BEFORE_DELETE_CHILD,
    TREE_NOTIFY_AFTER_DELETE_CHILD,
    TREE_NOTIFY_SUBTREE_CHANGE
};

typedef bool (*tree_notify_func)(struct exec_context *ctx, enum tree_notify_type type, ...);

typedef struct tree_node {
    apr_pool_t *scope;
    expr_value value;
    apr_hash_t *attrs;
    struct tree_node *parent;
    tree_notify_func notifier;
    bool local;
} tree_node;

extern tree_node *new_tree_node(apr_pool_t *scope)
    ATTR_NONNULL ATTR_WARN_UNUSED_RESULT ATTR_MALLOC;

extern tree_node *add_tree_node_attr(struct exec_context *ctx, tree_node *node, 
                                     const uint8_t *key, bool replace)
    ATTR_NONNULL ATTR_MALLOC;
extern void delete_tree_node_attr(struct exec_context *ctx, tree_node *node, const uint8_t *key)
    ATTR_NONNULL_ARGS((1, 2));
extern tree_node *lookup_tree_node_attr(struct exec_context *ctx, tree_node *node, const uint8_t *key)
    ATTR_NONNULL;

extern void update_tree_node(struct exec_context *ctx, tree_node *node)
    ATTR_NONNULL;

extern void set_tree_node_value(struct exec_context *ctx, tree_node *dest, expr_value src)
    ATTR_NONNULL;

extern void add_tree_node_list_value(struct exec_context *ctx, tree_node *dest, expr_value src)
    ATTR_NONNULL;

extern void set_tree_node_list_value(struct exec_context *ctx, tree_node *dest, 
                                     unsigned idx, expr_value src)
    ATTR_NONNULL;

extern void copy_tree_node(struct exec_context *ctx, tree_node *dest, tree_node *src)
    ATTR_NONNULL;

extern expr_value resolve_reference(expr_value val, bool lvalue)
    ATTR_PURE;

extern int compare_values(struct exec_context *ctx, bool inexact, expr_value v1, expr_value v2, bool ordering)
    ATTR_NONNULL_1ST;

typedef struct dispatch {
    struct {
        bool leaf;
        union {
            void *func;
            const struct dispatch *next;
        } x;
    } types[VALUE_MAX_TYPE + 1];
} dispatch;

extern void *dispatch_values(struct exec_context *ctx, const dispatch *disp, 
                             unsigned nargs, const expr_value *vals)
    ATTR_NONNULL ATTR_WARN_UNUSED_RESULT ATTR_PURE;


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* VALUES_H */
