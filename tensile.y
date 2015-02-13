%locations
%error-verbose

%{
#include "engine.h"
%}

%union {
    uint8_t *str;
    expr_value value;
    expr_node *expr;
    enum aggregate_type aggr;
    condition *cond;
    const predicate_def *pred;
    apr_array_header_t *list;
}
%token<value> TOK_BINARY
%token TOK_DEFAULT
%token<str> TOK_FIELD
%token TOK_GE
%token TOK_GE_CI
%token TOK_GREATER_CI
%token<str> TOK_ID
%token TOK_IMPORT
%token TOK_LE
%token TOK_LE_CI
%token TOK_LESS_CI
%token TOK_MATCH_CI
%token TOK_MIN 
%token TOK_MIN_CI
%token TOK_MAX 
%token TOK_MAX_CI
%token TOK_NE
%token TOK_NE_CI
%token TOK_NOT_MATCH
%token TOK_NOT_MATCH_CI
%token<value> TOK_NULL
%token<value> TOK_NUMBER
%token TOK_POST
%token TOK_PRE
%token<value> TOK_STRING
%token<value> TOK_TIMESTAMP
%token TOK_UMINUS
%token TOK_VARFIELD

%left '|'
%left ','
%nonassoc '='
%nonassoc '<' '>' '~' TOK_EQ TOK_NE TOK_EQ_CI TOK_NE_CI TOK_LESS_CI TOK_GREATER_CI TOK_LE_CI TOK_GE_CI TOK_NOT_MATCH TOK_MATCH_CI TOK_NOT_MATCH_CI
%nonassoc ':'
%left '@'
%left '&'
%left TOK_MAX TOK_MIN TOK_MAX_CI TOK_MIN_CI '$'
%left '+' '-'
%left '*' '/' '%'
%right '^'
%right '!' '#' TOK_UMINUS
%left TOK_FIELD TOK_VARFIELD '['

%type<list> exprlist
%type<list> exprlist0

%type<expr> expression
%type<aggr> aggregate
%type<expr> fieldsel
%type<expr> rangesel
%type<value> literal
%type<pred> relop
%type<pred> orderop
%type<pred> sortop
%type<expr> sortspec

%%

script:         imports body
                ;

imports:        /*empty */
        |       imports TOK_IMPORT importspec ';'
                ;

importspec:     TOK_ID
        |       importspec TOK_FIELD
                ;

body:           /*empty */
        |       body ';'
        |       body assignment ';'
        |       body stage
        |       body macrodef ';'
                ;

macrodef:       TOK_ID '(' macroargs ')' '=' macrobody
                ;

macrobody:      actionlist 
        |       '('condition ')'
                ;

macroargs:      /*empty */
        |       vararg
        |       TOK_ID
        |       TOK_ID '=' expression
        |       TOK_ID ',' macroargs
        |       TOK_ID '=' expression ',' optmacroargs
                ;

optmacroargs:   TOK_ID '=' expression
        |       vararg
        |       TOK_ID '=' expression ',' optmacroargs
                ;

vararg:         TOK_ID '[' ']'
                ;

stage:          TOK_ID '{' stagebody '}'
                ;

stagebody:      /*empty */
        |       stagebody guard action
        |       stagebody ';'
                ;

guard:          section topcondition
                ;

section:        /*empty */
        |       TOK_PRE
        |       TOK_POST
                ;

topcondition:   TOK_DEFAULT
        |       condition
                ;

action:         ':'actionlist ';'
        |       '{'stagebody '}'
                ;

condition:      simple_condition
        | '('   condition ')'
        |       '!'condition %prec TOK_UMINUS
        |       condition ',' condition
        |       condition '|' condition
                ;

simple_condition:
                expression relop expression
        |       TOK_ID '(' exprlist ')'
                ;

actionlist:     simple_action
        |       actionlist ',' simple_action
                ;

simple_action:  assignment
        |       TOK_ID actionargs
                ;

actionargs:      /*empty */
        |       '('exprlist ')'
                ;

assignment:     expression '=' expression
        |       TOK_DEFAULT expression '=' expression
                ;

exprlist:       expression
        |       exprlist ',' expression
                ;

exprlist0:      /*empty */
        |       exprlist
                ;

expression:     literal { $$ = make_expr_node(EXPR_LITERAL, $1); }
        |       '{'exprlist0 '}' { $$ = make_expr_node(EXPR_LIST, $2); }
        |       TOK_ID { $$ = make_expr_node(EXPR_VARREF, $1); }
        |       '.' { $$ = make_expr_node(EXPR_CONTEXT); }
        |       fieldsel { 
            $$ = set_field_base($1, make_expr_node(EXPR_CONTEXT)); 
                }
        |       expression fieldsel {            
                $$ = set_field_base($2, $1);
                }
        |       expression '[' ']' {
                $$ = make_expr_node(EXPR_UNOP, expr_op_make_iter, $1);
                }
        |       expression '[' aggregate ']' {
            $$ = make_expr_node(EXPR_AGGREGATE, $1, $3);
                }
        |       expression '[' rangesel ']' {
                $3->x.base = $1;
                $$ = $3;
                }
        |       '+'expression %prec TOK_UMINUS {
            $$ = make_expr_node(EXPR_UNOP, expr_op_uplus, $2);
                }
        |       '-'expression %prec TOK_UMINUS {
            $$ = make_expr_node(EXPR_UNOP, expr_op_uminus, $2);
                }
        |       '*'expression %prec TOK_UMINUS {
                $$ = make_expr_node(EXPR_UNOP, expr_op_tostring, $2);
                }
        |       '/'expression {
                $$ = make_expr_node(EXPR_UNOP, expr_op_floor, $2);
                }
        |       '%'expression %prec TOK_UMINUS {
                $$ = make_expr_node(EXPR_UNOP, expr_op_frac, $2);
                }
        |       '~'expression %prec TOK_UMINUS{
                $$ = make_expr_node(EXPR_UNOP, expr_op_lowercase, $2);
                }
        |       '#'expression {
                $$ = make_expr_node(EXPR_UNOP, expr_op_length, $2);
                }
        |       sortop expression %prec TOK_UMINUS {
                }
        |       expression '+' expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_plus, $1, $3);
                }
        |       expression '-' expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_minus, $1, $3);
                }
        |       expression '*' expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_mul, $1, $3);
                }
        |       expression '/' expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_div, $1, $3);
                }
        |       expression '%' expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_rem, $1, $3);
                }
        |       expression '&' expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_join, $1, $3);
                }
        |       expression '^' expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_power, $1, $3);
                }
        |       expression '@' expression {
                $$ = make_expr_node(EXPR_ATCONTEXT, $1, $3);
                }
        |       expression sortop expression %prec '$' {
                }
        |       expression TOK_MIN expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_min, $1, $3);
                }
        |       expression TOK_MAX expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_max, $1, $3);
                }
        |       expression TOK_MIN_CI expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_min_ci, $1, $3);
                }
        |       expression TOK_MAX_CI expression {
                $$ = make_expr_node(EXPR_BINOP, expr_op_max_ci, $1, $3);
                }
                ;

aggregate:      '+' { $$ = AGGR_SUM; }
        |       '*' { $$ = AGGR_PRODUCT; }
        |       '&' { $$ = AGGR_JOIN; }
        |       '/' { $$ = AGGR_AVERAGE; }
        |       TOK_MIN { $$ = AGGR_MIN; }
        |       TOK_MAX { $$ = AGGR_MAX; }
        |       TOK_MIN_CI { $$ = AGGR_MIN_CI; }
        |       TOK_MAX_CI { $$ = AGGR_MAX_CI; }
                ;


fieldsel:       TOK_FIELD {
                $$ = make_expr_node(EXPR_FIELD, NULL, $1);
                }
        |       TOK_VARFIELD ']' {
                $$ = make_expr_node(EXPR_UNOP, expr_op_children, NULL)
                }
        |       TOK_VARFIELD '#' ']'{
                $$ = make_expr_node(EXPR_UNOP, expr_op_count, NULL)
                }
        |       TOK_VARFIELD expression ']' {
                $$ = make_expr_node(EXPR_UNOP, expr_op_dyn_field, NULL, $2)
                }
                ;

rangesel:       expression {
                $$ = make_expr_node(EXPR_RANGE, NULL, $1, $1);
                }
        |       expression ':' expression {
                $$ = make_expr_node(EXPR_RANGE, NULL, $1, $3);
                }
        |       expression ':'
                {
                    $$ = make_expr_node(EXPR_RANGE, NULL, $1, 
                make_expr_node(EXPR_LITERAL, 
                (expr_value){.type = VALUE_NUMBER, .v = {.num = -1.0}}));
                }
        |       ':'expression
                {
                    $$ = make_expr_node(EXPR_RANGE, NULL, 
                make_expr_node(EXPR_LITERAL, 
                (expr_value){.type = VALUE_NUMBER, .v = {.num = 0.0}}),
                $2);
                }
                ;

literal:        TOK_STRING
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
        |       orderop { $$ = $1; }
        ;

orderop:              '<' {$$ = &predicate_match_less; }
        |       '>' {$$ = &predicate_match_greater; }
        |       TOK_LE {$$ = &predicate_match_ci; }
        |       TOK_GE
        |       TOK_LESS_CI
        |       TOK_GREATER_CI
        |       TOK_LE_CI
        |       TOK_GE_CI
                ;

sortop:         '$' sortspec
        |       '$' '!' sortspec
                ;

sortspec:       orderop
        |       TOK_ID
                ;
