%define api.pure true
%locations
%error-verbose
%parse-param {exec_context *context}
%lex-param {exec_context *context}

%{
#include <stdio.h>
#include "engine.h"
#include "ops.h"

static action *convert_to_check(exec_context *ctx, const expr_node *expr)
ATTR_NONNULL ATTR_MALLOC ATTR_WARN_UNUSED_RESULT;

static inline ATTR_NONNULL ATTR_WARN_UNUSED_RESULT ATTR_MALLOC
expr_node *make_literal_string(exec_context *context, uint8_t *str)
{
    return make_expr_node(context, EXPR_LITERAL, MAKE_VALUE(STRING, context->static_pool, str));
}

static inline
expr_node *make_closure_expr(exec_context *context, const action_def *def, apr_array_header_t *arglist)
{
    expr_node *head = make_expr_node(context, EXPR_LITERAL, make_closure(context, def, 0));
 
    if (arglist == NULL)
       return head;

    augment_arg_list(context, def, arglist, false);
    return make_expr_node(context, EXPR_OPERATOR, expr_op_bind, head,
                          make_expr_node(context, EXPR_LIST, arglist));
}


%}

%union {
    uint8_t *str;
    expr_value value;
    expr_node *expr;
    const action_def *actdef;
    apr_array_header_t *list;
    int ival;
    const operator_info *oper;
    action_func actfun;
    action *act;
    enum stage_section section;
    stage_contents sc;
    sectioned_action sact;
    stage *stage;
    void *gptr;
}
%token TOK_AS
%token TOK_CONST
%token TOK_ARROW
%token<value> TOK_BINARY
%token TOK_DEFAULT
%token TOK_EXTERN
%token<oper> TOK_COMP_ASSIGN
%token<str> TOK_FIELD
%token TOK_GE
%token TOK_GE_CI
%token TOK_GREATER_CI
%token<str> TOK_ID
%token TOK_IDENTITY
%token TOK_LE
%token TOK_LE_CI
%token TOK_LESS_CI
%token TOK_LOCAL
%token TOK_MATCH_CI
%token TOK_MIN 
%token TOK_MIN_CI
%token TOK_MAX 
%token TOK_MAX_CI
%token TOK_NE
%token TOK_NE_CI
%token TOK_NOT_IDENTITY
%token TOK_NOT_MATCH
%token TOK_NOT_MATCH_CI
%token<value> TOK_NULL
%token<value> TOK_NUMBER
%token TOK_PARENT
%token TOK_POST
%token TOK_PRAGMA
%token TOK_PRE
%token<str> TOK_STRING
%token TOK_THIS
%token<value> TOK_TIMESTAMP
%token TOK_TYPEOF
%token TOK_UMINUS
%token TOK_VARFIELD

%left ';'
%left '|'
%left ','
%nonassoc '=' TOK_DFL_ASSIGN TOK_ADD_ASSIGN TOK_JOIN_ASSIGN TOK_ARROW
%nonassoc '<' '>' '~' TOK_EQ TOK_NE TOK_EQ_CI TOK_NE_CI TOK_LESS_CI TOK_GREATER_CI TOK_LE_CI TOK_GE_CI TOK_NOT_MATCH TOK_MATCH_CI TOK_NOT_MATCH_CI
%nonassoc ':'
%left '?'
%left '@'
%left '&'
%left TOK_MAX TOK_MIN TOK_MAX_CI TOK_MIN_CI '$'
%left '+' '-'
%left '*' '/' '%'
%right '^'
%left TOK_AS
%right '`' '!' '#' TOK_UMINUS TOK_TYPEOF
%right TOK_EXTERN
%left TOK_FIELD TOK_VARFIELD '[' TOK_PARENT TOK_CHILDREN TOK_COUNT

%type<list> exprlist
%type<list> exprlist0
%type<list> attributes
%type<list> closure

%type<expr> assignment
%type<expr> expression
%type<oper> aggregate
%type<expr> fieldsel
%type<expr> rangesel
%type<value> literal
%type<actdef> relop
%type<actdef> orderop
%type<expr> sortspec
%type<expr> filterspec
%type<value> num0

%type<act> action
%type<act> simple_action
%type<act> topaction
%type<act> assignments0
%type<act> rhs
%type<act> innerstagebody
%type<sc> innerrules
%type<sc> innerrule

%type<str> gid
%type<str> gidnull
%type<str> gid0

%type<section> section

%type<stage> stage
%type<stage> stagebody
%type<stage> rules

%type<sact> rule

%{
extern int yylex(YYSTYPE* yylval_param, YYLTYPE * yylloc_param, exec_context *context);
static void yyerror(YYLTYPE * yylloc_param, exec_context *context, const char *msg);
%}

%%

script:         body
                ;

body:           /*empty */
        |       body ';'
        |       body assignment ';' { evaluate_expr_node(context, $2, false); }
        |       body stage
        |       body actiondef ';'
        |       body pragma ';'
                ;

actiondef:      gid  '(' macroargs0 ')' actionbody
        |       TOK_EXTERN gid '=' externsym
                ;

externsym:      TOK_FIELD
        |       gid TOK_FIELD
        ;

actionbody:     '=' action
        |       TOK_ARROW gidnull
        ;

gidnull:        gid
        |       TOK_NULL { $$ = NULL; }
        ;

macroargs0:     /*              empty */
        |       macroargs
                ;

macroargs:      vararg
        |       fmacroargs ',' vararg
        |       fmacroargs
                ;


fmacroargs:     argument
        |       fmacroargs ',' argument
                ;

argument:       TOK_ID
        |       TOK_ID '=' expression
        ;


vararg:         TOK_ID '[' ']'
                ;

gid:            TOK_ID
        |       TOK_STRING
        ;

stage:          TOK_ID '{' stagebody '}' 
                { $3->name = $1; $$ = $3; }
                ;

stagebody:      assignments0 rules 
                {
                    $2->contents[SECTION_PRE].normal = seq_actions(context, $1, $2->contents[SECTION_PRE].normal);
                }
                ;

assignments0:    /*              empty */  { $$ = NULL; }
        |       assignments0 assignment ';' 
                { 
                    $$ = seq_actions(context, $1, make_action(context, ACTION_EXPRESSION, $2)); 
                }
        |       assignments0 pragma ';' { $$ = $1; }
        ;

rules:          rule 
                {
                    $$ = make_stage(context); 
                    add_rule(context, $$, $1);
                }
        |       rules ';'
                {
                    $$ = $1;
                }
        |       rules rule %prec ';'
                {
                    add_rule(context, $1, $2);
                    $$ = $1;
                }
        ;

section:       TOK_PRE { $$ = SECTION_PRE; }
        |      TOK_POST { $$ = SECTION_POST; }
                ;


topaction:   TOK_DEFAULT { $$ = NULL; }
        |       action { $$ = $1; }
                ;

rule:           topaction rhs
                {
                    if ($1) {
                        $$ = (sectioned_action){SECTION_MAIN, 
                {.normal = make_action(context, ACTION_CONDITIONAL, $1, $2)}};
                }
                else
                {
                    $$ = (sectioned_action){SECTION_MAIN, {.otherwise = $2}};
                }
                }
        |       section topaction rhs
                {
                    if ($2) {
                        $$ = (sectioned_action){$1, 
                {.normal = make_action(context, ACTION_CONDITIONAL, $2, $3)}};
                }
                else
                {
                    $$ = (sectioned_action){$1, {.otherwise = $3}};
                }
                }
                
        ;

rhs:         ':'action ';' { $$ = $2; }
        |       '{'innerstagebody '}' 
                { 
                    $$ = make_action(context, ACTION_SCOPE, $2); 
                }
                ;

innerstagebody:      assignments0 innerrules
                { 
                    action *joint = alt_actions(context, $2.normal, $2.otherwise);
                    $$ = seq_actions(context, $1, joint);
                }
                ;

innerrules:     innerrule  { $$ = $1; }
        |       innerrules ';' { $$ = $1; }
        |       innerrules innerrule %prec ';'
                { 
                    $$  = combine_stage_contents(context, $1, $2); 
                }
        ;

innerrule:      topaction rhs
                { 
                    $$ = $1 ? (stage_contents){.normal = make_action(context, ACTION_CONDITIONAL, $1, $2)} : 
                    (stage_contents){.otherwise = $2}; 
                }
        ;

action:         simple_action
        |       '(' action ')' { $$ = $2; }
        |       '!'action %prec TOK_UMINUS
                {
                    $$ = make_action(context, ACTION_NOT, $2);
                }
        |       action ',' action
                {
                    $$ = seq_actions(context, $1, $3);
                }
        |       action '|' action
                {
                    $$ = alt_actions(context, $1, $3);
                }
                ;

simple_action:  assignment
                {
                    $$ = make_action(context, ACTION_EXPRESSION, $1);
                }
        |       expression relop expression %prec '<'
                {
                    $$ = make_relation(context, $2, $1, $3);
                }
        |       '?' expression
                {
                    $$ = convert_to_check(context, $2);
                }
        |       TOK_ID '(' exprlist0 ')'
                {
                    $$ = make_action(context, ACTION_CALL,
                                     lookup_action(context, $1),
                                     $3);
                }
                ;

assignment:     expression '=' expression
                {
                    $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_assignment, $3, $1);
                }
        |       expression TOK_ADD_ASSIGN expression 
                {
                    $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_assignment,
                                        make_expr_node(context, EXPR_OPERATOR, 
                                                       expr_op_plus, $1, $3), $1);
                }
        |       expression TOK_JOIN_ASSIGN expression 
                {
                    $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_assignment,
                                        make_expr_node(context, EXPR_OPERATOR, 
                                                       expr_op_join, $1, $3), $1);
                }
        |       expression TOK_DFL_ASSIGN expression 
                {
                    $$ = make_expr_node(context, EXPR_DEFAULT, 
                make_expr_node(context, EXPR_OPERATOR, expr_op_noop, $1),
                make_expr_node(context, EXPR_OPERATOR, 
                                                       expr_op_assignment, $3, $1));
                }
        |       expression TOK_ARROW expression
                {
                    $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_link, $1, $3);
                }
        |       TOK_CONST expression '=' expression
                {
                    $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_const, $2, $4);
                }
        |       TOK_LOCAL expression '=' expression
                {
                    $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_local_assign, $2, $4);
                }
                ;


pragma:         TOK_PRAGMA TOK_ID gid0 num0
        ;

gid0:         gid
        |    /* empty */
                { $$ = NULL; }
        ;

num0:           TOK_NUMBER
        |       /*empty */
                { $$ = MAKE_NUM_VALUE(0.0); }
        ;

exprlist:       expression { 
                $$ = make_expr_list(context);
                APR_ARRAY_PUSH($$, expr_node *) = $1;
                }
        |        ',' expression {
                $$ = make_expr_list(context);
                APR_ARRAY_PUSH($$, expr_node *) = NULL;
                APR_ARRAY_PUSH($$, expr_node *) = $2;
                }
        |       exprlist ',' expression {
                $$ = $1;
                APR_ARRAY_PUSH($$, expr_node *) = $3;
            }
        |       exprlist ','  {
                $$ = $1;
                APR_ARRAY_PUSH($$, expr_node *) = NULL;
            }
                ;

exprlist0:      /*empty */ { $$ = make_expr_list(context); }
        |       exprlist { $$ = $1; }
                ;

expression:     literal { $$ = make_expr_node(context, EXPR_LITERAL, $1); }
        |       '[' exprlist0 ']' { $$ = make_expr_node(context, EXPR_LIST, $2); }
        |       expression '<' attributes '>' {} %prec TOK_FIELD {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_makenode, $1, $3);
                }
        |       '(' expression ')' { $$ = $2; }
        |       TOK_ID { $$ = make_expr_node(context, EXPR_VARREF, $1); }
        |       TOK_THIS { $$ = make_expr_node(context, EXPR_CONTEXT); }
        |       fieldsel { 
                $$ = $1;
                $$->x.op.args[0] = make_expr_node(context, EXPR_CONTEXT); 
                }
        |       expression fieldsel {
                $$ = $2;
                $$->x.op.args[0] = $1; 
                }
        |       expression '[' ']' {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_make_iter, $1);
                }
        |       expression '[' aggregate ']' {
                $$ = make_expr_node(context, EXPR_OPERATOR, $3, $1);
                }
        |       expression '[' rangesel ']' {
                $$ = $3;
                $$->x.op.args[0] = $1; 
                }
        |       '+'expression %prec TOK_UMINUS {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_uplus, $2);
                }
        |       '-'expression %prec TOK_UMINUS {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_uminus, $2);
                }
        |       '*'expression %prec TOK_UMINUS {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_noop, $2);
                }
        |       '&'expression %prec TOK_UMINUS {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_lvalue, $2);
                }
        |       '/'expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_floor, $2);
                }
        |       '%'expression %prec TOK_UMINUS {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_frac, $2);
                }
        |       TOK_TYPEOF expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_typeof, $2);
                }
        |       '~'expression %prec TOK_UMINUS{
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_loosen, $2);
                }
        |       '#'expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_length, $2);
                }
        |       '$' sortspec expression %prec TOK_UMINUS {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_sort, $2, $3);
                }
        |       '`'filterspec {
                $$ = make_expr_node(context, EXPR_LITERAL, $2);
                }
        |       expression '+' expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_plus, $1, $3);
                }
        |       expression '-' expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_minus, $1, $3);
                }
        |       expression '*' expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_mul, $1, $3);
                }
        |       expression '/' expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_div, $1, $3);
                }
        |       expression '%' expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_rem, $1, $3);
                }
        |       expression '&' expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_join, $1, $3);
                }
        |       expression '^' expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_power, $1, $3);
                }
        |       expression '@' expression {
                $$ = make_expr_node(context, EXPR_AT_CONTEXT, $1, $3);
                }
        |       expression '?' expression {
                $$ = make_expr_node(context, EXPR_DEFAULT, $1, $3);
                }
        |       expression '$' sortspec expression %prec '$' {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_merge, $1, $3, $4);
                }
        |       expression TOK_MIN expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_min, $1, $3);
                }
        |       expression TOK_MAX expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_max, $1, $3);
                }
        |       expression TOK_MIN_CI expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_min_ci, $1, $3);
                }
        |       expression TOK_MAX_CI expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_max_ci, $1, $3);
                }
        |       expression TOK_AS gid {
                $$ = make_expr_node(context, EXPR_TYPECAST, $1, $3);
                }
                ;

attributes:     TOK_FIELD ':' expression {
                $$ = make_expr_list(context);
                APR_ARRAY_PUSH($$, expr_node *) = make_literal_string(context, $1);
                APR_ARRAY_PUSH($$, expr_node *) = $3;
                }
        |       attributes ',' TOK_FIELD '=' expression
                {
                    $$ = $1;
                    APR_ARRAY_PUSH($$, expr_node *) = make_literal_string(context, $3);
                    APR_ARRAY_PUSH($$, expr_node *) = $5;
                }
        ;

aggregate:      '+' { $$ = &expr_op_aggregate_sum; }
        |       '*' { $$ = &expr_op_aggregate_prod; }
        |       '&' { $$ = &expr_op_aggregate_join; }
        |       '/' { $$ = &expr_op_aggregate_avg; }
        |       TOK_MIN { $$ = &expr_op_aggregate_min; }
        |       TOK_MAX { $$ = &expr_op_aggregate_max; }
        |       TOK_MIN_CI { $$ = &expr_op_aggregate_min_ci; }
        |       TOK_MAX_CI { $$ = &expr_op_aggregate_max_ci; }
                ;


fieldsel:       TOK_FIELD {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_getfield, NULL, 
                                    make_literal_string(context, $1));
                }
        |       TOK_CHILDREN {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_children, NULL);
                }
        |       TOK_COUNT {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_count, NULL);
                }
        |       TOK_VARFIELD expression ']' {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_getfield, NULL, $2);
                }
        |       TOK_PARENT {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_parent, NULL);
                }
                ;

rangesel:       expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_item, NULL, $1);
                }
        |       expression ':' expression {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_range, NULL, $1, $3);
                }
        |       expression ':'
                {
                    $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_range, NULL, $1, 
                make_expr_node(context, EXPR_LITERAL, MAKE_NUM_VALUE(INFINITY)));

                }
        |       ':'expression
                {
                $$ = make_expr_node(context, EXPR_OPERATOR, expr_op_range, NULL, 
                make_expr_node(context, EXPR_LITERAL, MAKE_NUM_VALUE(0.0)),
                                    $2);
                }
                ;

literal:        TOK_STRING { $$ = MAKE_VALUE(STRING, context->static_pool, $1); }
        |       TOK_BINARY
        |       TOK_NUMBER
        |       TOK_TIMESTAMP
        |       TOK_NULL
                ;

relop:          TOK_EQ { $$ = &predicate_eq; }
        |       TOK_NE { $$ = &predicate_ne; }
        |       TOK_EQ_CI { $$ = &predicate_eq_ci; }
        |       TOK_NE_CI { $$ = &predicate_ne_ci; }
        |       '~' {$$ = &predicate_match; }
        |       TOK_NOT_MATCH { $$ = &predicate_not_match; }
        |       TOK_MATCH_CI {$$ = &predicate_match_ci; }
        |       TOK_NOT_MATCH_CI {$$ = &predicate_not_match_ci; }
        |       TOK_IDENTITY {$$ = &predicate_identity; }
        |       TOK_NOT_IDENTITY {$$ = &predicate_not_identity; }
        |       orderop { $$ = $1; }
        ;

orderop:              '<' {$$ = &predicate_less; }
        |       '>' {$$ = &predicate_greater; }
        |       TOK_LE {$$ = &predicate_le; }
        |       TOK_GE {$$ = &predicate_ge; }
        |       TOK_LESS_CI {$$ = &predicate_less_ci; }
        |       TOK_GREATER_CI {$$ = &predicate_greater_ci; }
        |       TOK_LE_CI {$$ = &predicate_le_ci; }
        |       TOK_GE_CI {$$ = &predicate_ge_ci; }
                ;

sortspec:       orderop 
                { 
                    $$ = make_closure_expr(context, $1, NULL);
                }
        |       TOK_ID 
                {
                    $$ = make_closure_expr(context, lookup_action(context, $1), NULL);
                }
        |       '(' expression ')' { $$ = $2; }
                ;

filterspec:     relop closure 
                {
                    $$ = make_closure_expr(context, $1, $2);
                }
        |       TOK_ID closure 
                {
                    $$ = make_closure_expr(context, lookup_action(context, $1), $2);
                }
        ;

closure:         /*              empty */ { $$ = NULL; }
        |       '(' ')' { $$ = NULL; }
        |       '(' exprlist ')' { $$ = $2; }
        ;

%%

static action *convert_to_check(exec_context *context, const expr_node *expr)
{
    switch (expr->type)
    {
        case EXPR_VARREF:
            return make_call(context, &predicate_has_var,
                             make_literal_string(context, expr->x.varname),
                             NULL);
        case EXPR_OPERATOR:
            if (expr->x.op.func == &expr_op_getfield)
            {
                return make_relation(context, &predicate_has_field, 
                                     expr->x.op.args[0], expr->x.op.args[1]);
            }
            if (expr->x.op.func == &expr_op_children)
            {
                return make_call(context, &predicate_has_children,
                                 expr->x.op.args[0], NULL);
            }
            if (expr->x.op.func == &expr_op_parent)
            {
                return make_call(context, &predicate_has_parent,
                                 expr->x.op.args[0], NULL);
            }
            /* fallthrough */
        default:
            raise_error(context, TEN_UNRECOGNIZED_CHECK);
    }
    return NULL;
}

static void yyerror(YYLTYPE * yylloc_param, exec_context *context ATTR_UNUSED, const char *msg)
{
    const char *fname = (const char *)APR_ARRAY_IDX(context->conf->lexer_buffers,
                                                    context->conf->lexer_buffers->nelts - 1,
                                                    lexer_buffer).filename;
    fprintf(stderr, "%s:%d: %s\n", fname ? fname : "<stream>", 
            yylloc_param->first_line, msg);
}
