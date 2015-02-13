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
 * @brief Basic engine definitions
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef ENGINE_H
#define ENGINE_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>
#include <unistr.h>
#include <apr_hash.h>
#include <apr_table.h>

enum value_type {
    VALUE_NULL,
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_BINARY,
    VALUE_TIMESTAMP,
    VALUE_NODEREF,
    VALUE_LIST,
    VALUE_LIST_ITER,
};

typedef struct expr_value {
    enum value_type type;
    union {
        double num;
        uint8_t *str;
        time_t ts;
        struct {
            unsigned len;
            uint8_t *data;
        } bin;
        struct tree_node *ref;
        apr_array_header_t *list;
    } v;
} expr_value;

enum expr_type {
    EXPR_LITERAL,
    EXPR_VARREF,
    EXPR_LIST,
    EXPR_CONTEXT,
    EXPR_FIELD,
    EXPR_AGGREGATE,
    EXPR_RANGE,
    EXPR_UNOP,
    EXPR_BINOP,
    EXPR_ATCONTEXT,
    EXPR_FILTER,
    EXPR_SORT,
    EXPR_MERGE,
};

enum aggregate_type {
    AGGR_SUM,
    AGGR_PRODUCT,
    AGGR_JOIN,
    AGGR_AVERAGE,
    AGGR_MIN,
    AGGR_MAX,
    AGGR_MIN_CI,
    AGGR_MAX_CI
};

typedef expr_value (*expr_unop_func)(const expr_value *v);
typedef expr_value (*expr_binop_func)(const expr_value *v1, const expr_value *v2);

typedef struct expr_node {
    enum expr_type type;
    union {
        expr_value literal;
        uint8_t *varname;
        aprt_array_header_t *list;
        struct {
            struct expr_node *base;
            uint8_t *varname;
        } fsel;
        struct {
            struct expr_node *base;
            enum aggregate_type op;
        } aggr;
        struct {
            struct expr_node *base;
            struct expr_node *first;
            struct expr_node *last;
        } range;
        struct {
            expr_unop_func func;
            struct expr_node *arg;
        } unop;
        struct {
            expr_binop_func func;
            struct expr_node *arg1;
            struct expr_node *arg2;
        } binop;
        struct {
            const struct predicate_def *pred;
            bool negate;
            struct expr_node *arg1;
            struct expr_node *arg2;
        } sortfilter;
    } x;
} expr_node;

extern expr_node *make_expr_node(enum expr_type t, ...);

enum condition_type {
    COND_PREDICATE,
    COND_NEGATE,
    COND_AND,
    COND_OR
};

typedef struct condition {
    enum condition_type type;
    union {
        struct {
            const struct predicate_def *pred;
            apr_array_header_t *args;
        } p;
        struct condition *arg;
        struct {
            struct condition *arg1;
            struct condition *arg2;
        } bin;
    } c;
} condition;

typedef struct var_bindings {
    apr_hash_t *binding;
    struct var_bindings *chain;
} var_bindings;



typedef struct signature {
    unsigned req_args;
    unsigned opt_args;
    bool vararg;
    uint8_t **names;
    expr_node **defaults;
} signature;


typedef bool (*predicate_func)(const var_bindings *vars, 
                               const struct tree_node *ctx,
                               apr_array_header_t *args);

typedef struct predicate_def {
    signature sig;
    bool is_user;
    union {
        predicate_func internal;
        condition *user;
    } u;
} predicate_def;

enum tree_notify_type {
    TREE_NOTIFY_CHANGE_VALUE,
    TREE_NOTIFY_ADD_CHILD,
    TREE_NOTIFY_DELETE_CHILD,
    TREE_NOTIFY_SUBTREE_CHANGE
};

typedef bool (*tree_notify_func)(enum tree_notify_type type, ...);

typedef struct tree_node {
    apt_hash_t *attrs;
    struct tree_node *parent;
    tree_notify_func notifier;
    expr_value value;
} tree_node;

tree_node *lookup_var(const var_bindings *vars, uint8_t *name);

expr_value evaluate_expr_node(const var_bindings *vars, 
                              struct tree_node *ctx, 
                              const expr_node *expr);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ENGINE_H */
