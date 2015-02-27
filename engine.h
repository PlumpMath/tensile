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

#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <setjmp.h>
#include <unistr.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include <apr_tables.h>

#include "support.h"

typedef struct engine_conf {
    double verbosity;
    bool assertions;
    apr_hash_t *loaded_dso;
    apr_hash_t *imported;
    apr_hash_t *actions;
    struct var_bindings *global_vars;
} engine_conf;

typedef enum stage_section {
    SECTION_PRE,
    SECTION_MAIN,
    SECTION_POST,
    SECTION_MAX
} stage_section;

typedef struct exec_context {
    engine_conf *conf;
    sigjmp_buf *exitpoint;
    apr_pool_t *static_pool;
    apr_pool_t *global_pool;
    apr_pool_t *stage_pool;
    apr_pool_t *local_pool;
    struct stage *current_stage;
    stage_section current_section;
    struct tree_node *context_node;
    struct var_bindings *bindings;
    apr_array_header_t *stage_queue;
} exec_context;

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

typedef struct binary_data {
    const struct binary_data_type *type;
    unsigned len;
    uint8_t *data;
} binary_data;

typedef struct closure {
    const struct action_def *def;
    apr_array_header_t *args;
} closure;

typedef struct expr_value {
    enum value_type type;
    apr_pool_t *scope;
    union {
        double num;
        uint8_t *str;
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

#define NULL_VALUE ((expr_value){.type = VALUE_NULL})

typedef struct binary_data_type {
    uint8_t *(*to_string)(exec_context *ctx, binary_data data);
    double (*to_number)(exec_context *ctx, binary_data data);
    expr_value (*loosen)(exec_context *ctx, binary_data data);
    expr_value (*reverse)(exec_context *ctx, binary_data data);
    size_t (*length)(exec_context *ctx, binary_data data);
    expr_value (*concat)(exec_context *ctx, binary_data data, binary_data other);
    expr_value (*strip)(exec_context *ctx, binary_data data, binary_data other);
    expr_value (*intersperse)(exec_context *ctx, binary_data data, binary_data other);
    expr_value (*split)(exec_context *ctx, binary_data data, const expr_value *optarg);
    expr_value (*tokenize)(exec_context *ctx, binary_data data, const expr_value *optarg);
    expr_value (*item)(exec_context *ctx, binary_data data, expr_value arg);
    expr_value (*range)(exec_context *ctx, binary_data data, expr_value arg1, expr_value arg2);
    int (*compare)(exec_context *ctx, bool inexact, binary_data data1, binary_data data2);
    bool (*match)(exec_context *ctx, bool inexact, binary_data data, expr_value pattern);
} binary_data_type;


enum expr_type {
    EXPR_LITERAL,
    EXPR_VARREF,
    EXPR_LIST,
    EXPR_CONTEXT,
    EXPR_OPERATOR
};

typedef struct expr_node expr_node;

typedef expr_value (*expr_operator_func)(exec_context *ctx, bool lvalue, unsigned nv, 
                                         expr_node **vs);

struct expr_node {
    enum expr_type type;
    union {
        expr_value literal;
        uint8_t *varname;
        apr_array_header_t *list;
        struct {
            expr_operator_func func;
            unsigned nargs;
            struct expr_node *args[3];
        } op;
    } x;
};

extern expr_node *make_expr_node(exec_context *ctx, enum expr_type t, ...)
    ATTR_MALLOC
    ATTR_NONNULL_1ST;
extern apr_array_header_t *make_expr_list(exec_context *ctx)
    ATTR_MALLOC
    ATTR_NONNULL_1ST;

typedef struct variable {
    bool local;
    struct tree_node *node;
} variable;

typedef struct var_bindings {
    apr_pool_t *scope;
    apr_hash_t *binding;
    struct var_bindings *chain;
} var_bindings;

enum tree_notify_type {
    TREE_NOTIFY_UPDATE_VALUE,
    TREE_NOTIFY_CHANGE_VALUE,
    TREE_NOTIFY_BEFORE_ADD_CHILD,
    TREE_NOTIFY_AFTER_ADD_CHILD,
    TREE_NOTIFY_BEFORE_DELETE_CHILD,
    TREE_NOTIFY_AFTER_DELETE_CHILD,
    TREE_NOTIFY_SUBTREE_CHANGE
};

typedef bool (*tree_notify_func)(exec_context *ctx, enum tree_notify_type type, ...);

typedef struct tree_node {
    apr_pool_t *scope;
    apr_hash_t *attrs;
    struct tree_node *parent;
    tree_notify_func notifier;
    expr_value value;
} tree_node;

tree_node *lookup_var(exec_context *ctx, uint8_t *name, bool local) ATTR_NONNULL;

expr_value evaluate_expr_node(exec_context *ctx,
                              const expr_node *expr,
                              bool lvalue) ATTR_NONNULL;

typedef enum action_result {
    ACTION_PROCEED,
    ACTION_NEXTSTAGE,
    ACTION_BREAK,
    ACTION_CONTINUE
} action_result;

typedef action_result (*action_func)(exec_context *ctx, apr_array_header_t *args);

typedef struct action_def {
    unsigned n_args;
    bool vararg;
    uint8_t **names;
    expr_node **defaults;
    struct action *body;
} action_def;


enum action_type {
    ACTION_CALL,
    ACTION_EXPRESSION,
    ACTION_NOT,
    ACTION_SEQ,
    ACTION_ALT,
    ACTION_SCOPE,
    ACTION_CONDITIONAL
};

typedef struct action {
    enum action_type type;
    union {
        struct {
            const struct action_def *def;
            apr_array_header_t *args;
        } call;
        expr_node *expr;
        struct action *negated;
        apr_array_header_t *seq;
        apr_array_header_t *alt;
        struct {
            struct action *guard;
            struct action *body;
        } cond;
        struct action *scoped;
    } c;
} action;

extern action *make_action(exec_context *ctx, enum action_type type, ...) ATTR_NONNULL_1ST ATTR_MALLOC;
extern action *seq_actions(exec_context *ctx, action *arg1, action *arg2) ATTR_NONNULL_1ST;
extern action *alt_actions(exec_context *ctx, action *arg1, action *arg2) ATTR_NONNULL_1ST;
extern action *alt_def_actions(exec_context *ctx, action *arg1, action *arg2) ATTR_NONNULL_1ST;
extern action *make_relation(exec_context *ctx,
                             const action_def *rel, 
                             const expr_node *arg1, 
                             const expr_node *arg2) ATTR_NONNULL ATTR_MALLOC;
extern action *make_call(exec_context *ctx, 
                         const action_def *act, 
                         ...) ATTR_NONNULL_ARGS((1, 2)) ATTR_SENTINEL ATTR_MALLOC;

extern const action_def *lookup_action(exec_context *ctx, const uint8_t *name) ATTR_NONNULL;

extern const action_def *resolve_dso_sym(exec_context *ctx,
                                         const uint8_t *library, 
                                         const uint8_t *name) ATTR_NONNULL_ARGS((1, 3));

typedef struct stage_contents {
    action *normal;
    action *otherwise;
} stage_contents;

typedef struct stage {
    uint8_t *name;
    stage_contents contents[SECTION_MAX];
} stage;

extern stage *make_stage(exec_context *ctx) ATTR_NONNULL ATTR_MALLOC;


typedef struct sectioned_action {
    stage_section section;
    stage_contents what;
} sectioned_action;

extern void add_rule(exec_context *ctx, stage *st, sectioned_action act) 
    ATTR_NONNULL;

extern void raise_error(exec_context *ctx, apr_status_t code)
    ATTR_NORETURN ATTR_NONNULL;

#define TENSILE_ERR_SPACE (APR_OS_START_USERERR + 100500)

enum tensile_error_code {
    TEN_PARSE_ERROR = TENSILE_ERR_SPACE,
    TEN_BAD_TYPE,
    TEN_UNRECOGNIZED_CHECK
};


extern expr_value parse_iso_timestamp(exec_context *ctx, const char *ts) ATTR_NONNULL;
extern expr_value parse_net_timestamp(exec_context *ctx, const char *ts) ATTR_NONNULL;

extern expr_value decode_hex(exec_context *ctx, const char *hex) ATTR_NONNULL;

extern double parse_duration(exec_context *ctx, const char *dur) ATTR_NONNULL;

extern expr_value unquote_string(exec_context *ctx, const char *str) ATTR_NONNULL;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ENGINE_H */
