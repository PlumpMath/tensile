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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <setjmp.h>
#include <unistr.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include <apr_tables.h>

#include "support.h"
#include "values.h"

typedef struct engine_conf {
    double verbosity;
    bool assertions;
    apr_hash_t *loaded_dso;
    apr_hash_t *imported;
    apr_hash_t *actions;
    apr_hash_t *pragmas;
    apr_hash_t *typecasts;
    struct var_bindings *global_vars;
    apr_array_header_t *search_path;
    apr_array_header_t *dso_search_path;
    apr_array_header_t *lexer_buffers;
} engine_conf;

typedef enum stage_section {
    SECTION_PRE,
    SECTION_MAIN,
    SECTION_POST,
    SECTION_MAX
} stage_section;

typedef struct call_frame {
    struct action_def *called;
    unsigned n_args;
    const expr_value *args;
    struct call_frame *up;
} call_frame;

typedef struct lexer_buffer {
    void *buffer_state;
    unsigned lineno;
    const uint8_t *modulename;
    const uint8_t *filename;
    double req_version;
    FILE *stream;
} lexer_buffer;

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
    call_frame *frame;
} exec_context;

typedef struct binary_data_type {
    const char *name;
    uint8_t *(*to_string)(exec_context *ctx, binary_data data);
    double (*to_number)(exec_context *ctx, binary_data data);
    expr_value (*loosen)(exec_context *ctx, binary_data data);
    expr_value (*uplus)(exec_context *ctx, binary_data data);
    expr_value (*reverse)(exec_context *ctx, binary_data data);
    size_t (*length)(exec_context *ctx, binary_data data);
    expr_value (*concat)(exec_context *ctx, binary_data data, binary_data other);
    expr_value (*strip)(exec_context *ctx, binary_data data, binary_data other);
    expr_value (*repeat)(exec_context *ctx, binary_data data, double num);
    expr_value (*intersperse)(exec_context *ctx, binary_data data, binary_data other);
    expr_value (*split)(exec_context *ctx, binary_data data, const expr_value *optarg);
    expr_value (*tokenize)(exec_context *ctx, binary_data data, const expr_value *optarg);
    expr_value (*item)(exec_context *ctx, binary_data data, expr_value arg);
    expr_value (*range)(exec_context *ctx, binary_data data, expr_value arg1, expr_value arg2);
    int (*compare)(exec_context *ctx, bool inexact, binary_data data1, binary_data data2, bool ordering);
    bool (*match)(exec_context *ctx, bool inexact, binary_data data, expr_value pattern);
    void (*aftercopy)(exec_context *ctx, expr_value *copy);
} binary_data_type;


extern const binary_data_type default_binary_data;

enum expr_type {
    EXPR_LITERAL,
    EXPR_VARREF,
    EXPR_LIST,
    EXPR_CONTEXT,
    EXPR_OPERATOR,
    EXPR_TYPECAST,
    EXPR_AT_CONTEXT,
    EXPR_DEFAULT
};

typedef struct expr_node expr_node;

typedef expr_value (*expr_operator_func)(exec_context *ctx, bool lvalue, 
                                         unsigned nv, 
                                         expr_value *vs);

typedef expr_value (*typecast_func)(exec_context *ctx, expr_value src);

typedef struct arg_info {
    bool lvalue;
    bool iterable;
    bool product;
} arg_info;

typedef struct operator_info {
    unsigned n_args;
    dispatch *func; 
    const arg_info *info;
} operator_info;

struct expr_node {
    enum expr_type type;
    union {
        expr_value literal;
        uint8_t *varname;
        apr_array_header_t *list;
        struct {
            const operator_info *func;
            const struct expr_node *args[3];
        } op;
        struct {
            typecast_func func;
            const struct expr_node *arg;
        } cast;
        struct 
        {
            const struct expr_node *context;
            const struct expr_node *arg;
        } at;
    } x;
};

extern expr_node *make_expr_node(exec_context *ctx, enum expr_type t, ...)
    ATTR_MALLOC
    ATTR_NONNULL_1ST
    ATTR_WARN_UNUSED_RESULT;

static inline ATTR_MALLOC ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT
apr_array_header_t *make_expr_list(exec_context *ctx)
{
    return apr_array_make(ctx->static_pool, 0, sizeof(expr_node *));
}

typedef struct var_bindings {
    apr_pool_t *scope;
    apr_hash_t *binding;
    struct var_bindings *chain;
} var_bindings;

tree_node *lookup_var(exec_context *ctx, const uint8_t *name, bool local) ATTR_NONNULL ATTR_WARN_UNUSED_RESULT;
tree_node *define_var(exec_context *ctx, const uint8_t *name) ATTR_NONNULL;

extern expr_value evaluate_expr_node(exec_context *ctx,
                                     const expr_node *expr,
                                     bool lvalue) 
    ATTR_NONNULL;

extern expr_value typecast_tostring(exec_context *ctx, expr_value val);

typedef enum action_result {
    ACTION_PROCEED,
    ACTION_NEXTSTAGE,
    ACTION_BREAK,
    ACTION_CONTINUE
} action_result;

typedef action_result (*action_func)(exec_context *ctx, unsigned n, expr_value *args);
typedef void (*pragma_func)(exec_context *ctx, const uint8_t *name, 
                            const uint8_t *strval, double numval);

typedef struct action_def {
    unsigned n_args;
    bool vararg;
    uint8_t **names;
    const expr_node **defaults;
    const arg_info *info;
    bool external;
    union {
        struct action *body;
        dispatch *func;
    } x;
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

extern void augment_arg_list(exec_context *ctx, const action_def *def,
                             apr_array_header_t *args, bool complete)
    ATTR_NONNULL;

extern action *make_action(exec_context *ctx, enum action_type type, ...) 
    ATTR_NONNULL_1ST ATTR_MALLOC
    ATTR_WARN_UNUSED_RESULT;
extern action *seq_actions(exec_context *ctx, action *arg1, action *arg2) 
    ATTR_NONNULL_1ST
    ATTR_WARN_UNUSED_RESULT;
extern action *alt_actions(exec_context *ctx, action *arg1, action *arg2) 
    ATTR_NONNULL_1ST
    ATTR_WARN_UNUSED_RESULT;

extern action *make_call(exec_context *ctx, 
                         const action_def *act, 
                         ...) 
    ATTR_NONNULL_ARGS((1, 2)) ATTR_SENTINEL ATTR_MALLOC
    ATTR_WARN_UNUSED_RESULT;

static inline ATTR_NONNULL ATTR_MALLOC ATTR_WARN_UNUSED_RESULT
action *make_relation(exec_context *ctx,
                      const action_def *rel, 
                      const expr_node *arg1, 
                      const expr_node *arg2) 
{
    return make_call(ctx, rel, arg1, arg2, NULL);
}


extern const action_def *lookup_action(exec_context *ctx, const uint8_t *name) 
    ATTR_NONNULL
    ATTR_WARN_UNUSED_RESULT;

extern const action_def *resolve_dso_sym(exec_context *ctx,
                                         const uint8_t *library, 
                                         const uint8_t *name) 
    ATTR_NONNULL_ARGS((1, 3))
    ATTR_WARN_UNUSED_RESULT;

typedef struct stage_contents {
    action *normal;
    action *otherwise;
} stage_contents;

static inline  ATTR_WARN_UNUSED_RESULT
stage_contents combine_stage_contents(exec_context *ctx, 
                                      stage_contents arg1,
                                      stage_contents arg2)
{
    return (stage_contents){
        .normal = alt_actions(ctx, arg1.normal, arg2.normal),
            .otherwise = seq_actions(ctx, arg1.otherwise, arg2.otherwise)
    };
}


typedef struct stage {
    uint8_t *name;
    stage_contents contents[SECTION_MAX];
} stage;

static inline ATTR_NONNULL ATTR_MALLOC ATTR_WARN_UNUSED_RESULT
stage *make_stage(exec_context *ctx)
{
    return apr_pcalloc(ctx->static_pool, sizeof(stage));
}


typedef struct sectioned_action {
    stage_section section;
    stage_contents what;
} sectioned_action;

static inline ATTR_NONNULL
void add_rule(exec_context *ctx, stage *st, sectioned_action act)
{
    st->contents[act.section] = combine_stage_contents(ctx, st->contents[act.section], act.what);
}

static inline ATTR_NORETURN ATTR_NONNULL
void raise_error(exec_context *ctx, apr_status_t code)
{
    siglongjmp(*ctx->exitpoint, code);
}

#define TENSILE_ERR_SPACE (APR_OS_START_USERERR + 100500)

enum tensile_error_code {
    TEN_PARSE_ERROR = TENSILE_ERR_SPACE,
    TEN_BAD_TYPE,
    TEN_UNRECOGNIZED_CHECK,
    TEN_OUT_OF_RANGE,
    TEN_TOO_FEW_ARGS,
    TEN_TOO_MANY_ARGS,
    TEN_NO_REQUIRED_ARG,
    TEN_DANGLING_REF
};

#define CHECKED_DIVIDE(_ctx, _num, _den) \
    ((_den) == 0 ? (raise_error((_ctx), APR_EINVAL), 0u) : (_num) / (_den))

extern expr_value parse_iso_timestamp(exec_context *ctx, const char *ts) 
    ATTR_NONNULL
    ATTR_WARN_UNUSED_RESULT;
extern expr_value parse_net_timestamp(exec_context *ctx, const char *ts) 
    ATTR_NONNULL
    ATTR_WARN_UNUSED_RESULT;

extern time_t timestamp_just_date(time_t ts)
    ATTR_WARN_UNUSED_RESULT;

extern expr_value decode_hex(exec_context *ctx, const char *hex) 
    ATTR_NONNULL
    ATTR_WARN_UNUSED_RESULT;

extern double parse_duration(exec_context *ctx, const char *dur) 
    ATTR_NONNULL
    ATTR_WARN_UNUSED_RESULT;

extern expr_value unquote_string(exec_context *ctx, const char *str) 
    ATTR_NONNULL
    ATTR_WARN_UNUSED_RESULT;

extern uint8_t *find_binary(binary_data data, binary_data search)
    ATTR_WARN_UNUSED_RESULT
    ATTR_PURE;

extern size_t normalize_index(exec_context *ctx, size_t len, double idx) ATTR_PURE;

extern void end_module_parsing(exec_context *ctx);
extern bool require_module(exec_context *ctx, const uint8_t *name, double version);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ENGINE_H */
