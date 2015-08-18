%define api.pure full
%locations
%error-verbose
%parse-param {exec_context *context}
%lex-param {exec_context *context}

%{
#include <stdio.h>
#include "engine.h"
#include "ops.h"

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

body:           /*empty*/
        |       body ';'
        |       body module
        |       body pragma ';'
        |       body toplevelconditional 
                ;

pragma:         TOK_PRAGMA TOK_ID literal
        ;


module:         TOK_MODULE TOK_ID version0 '{' modulecontents '}'
        ;

version0:       /*empty*/
        |       version
        ;

version:        TOK_FLOAT
        |       TOK_TIMESTAMP
        ;

toplevelconditional: 
                TOK_IF '(' tlmcondition ')' '{' body '}' toplevelelse
        ;

toplevelelse:   /*empty*/
        |       TOK_ELSE '{' body '}'
        ;

tlmcondition:   '(' tlmcondition ')'
        |       TOK_ID versionconstraint
        |       TOK_ID '.' TOK_ID
        |       TOK_ID '.' TOK_ID '.' TOK_ID
        |       '[' TOK_ID ']' envconstraint
        |       '!' tlmcondition
        |       tlmcondition '&' tlmcondition
        |       tlmcondition '|' tlmcondition
        |       tlmcondition '^' tlmcondition
        ;

versionconstrain: /*empty*/
        |       TOK_EQ version
        |       '>' version
        |       '<' version
        |       TOK_LE version
        |       TOK_GE version
        |       '~' version
        ;

envconstraint:  /*empty*/
        |       TOK_EQ TOK_STRING
        |       TOK_NE TOK_STRING
        ;

modulecontents:  /*empty*/
        |       modulecontents ';'
        |       modulecontents import ';'
        |       modulecontents foreign ';'
        |       modulecontents pragma ';'
        |       modulecontents moduleconditional
        |       modulecontents declaration 
        ;

import:         TOK_IMPORT importalias TOK_ID versionconstraint importlist 
        ;

importlist:     /*empty*/
        |       '(' idlist0 ')'
        ;

importalias:    /*empty*/
        |       TOK_ID TOK_ASSIGN
        ;

foreign:  TOK_FOREIGN TOK_ID TOK_ASSIGN TOK_STRING foreignsym
        ;

foreignsym:     /*empty*/
                '.' TOK_ID
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


static void yyerror(YYLTYPE * yylloc_param, exec_context *context ATTR_UNUSED, const char *msg)
{
    const char *fname = (const char *)APR_ARRAY_IDX(context->conf->lexer_buffers,
                                                    context->conf->lexer_buffers->nelts - 1,
                                                    lexer_buffer).filename;
    fprintf(stderr, "%s:%d: %s\n", fname ? fname : "<stream>", 
            yylloc_param->first_line, msg);
}
