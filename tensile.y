%locations
%error-verbose

%{
#include "engine.h"
%}

%token TOK_BINARY
%token TOK_DEFAULT
%token TOK_FIELD
%token TOK_GE
%token TOK_GE_CI
%token TOK_GREATER_CI
%token TOK_ID
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
%token TOK_NUMBER
%token TOK_POST
%token TOK_PRE
%token TOK_STRING
%token TOK_TIMESTAMP
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

%union {
    expr_value value;
    expr_node *expr;
    enum aggregate_type aggr;                    
}

%%

script:         imports body
        ;

imports:        /*              empty */
        |       imports TOK_IMPORT importspec ';'
        ;

importspec:     TOK_ID
        |       importspec TOK_FIELD
        ;

body:           /*              empty */
        |       body ';'
        |       body assignment ';'
        |       body stage
        |       body macrodef ';'
        ;

macrodef: TOK_ID '(' macroargs ')' '=' macrobody
        ;

macrobody: actionlist 
        |       '(' condition ')'
        ;

macroargs:      /*              empty */
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

stagebody:      /*              empty */
        |       stagebody guard action
        |       stagebody ';'
        ;

guard:          section topcondition
        ;

section:        /*              empty */
        |       TOK_PRE
        |       TOK_POST
        ;

topcondition:   TOK_DEFAULT
        |       condition
        ;

action:         ':' actionlist ';'
        |       '{' stagebody '}'
        ;

condition:      simple_condition
        | '('   condition ')'
        |       '!'condition %prec TOK_UMINUS
        |       condition ',' condition
        |       condition '|' condition
                ;

simple_condition: expression relop expression
        |       TOK_ID '(' exprlist ')'
        ;

actionlist:     simple_action
        |       actionlist ',' simple_action
        ;

simple_action:         assignment
        |       TOK_ID actionargs
        ;

actionargs:      /*              empty */
        |       '(' exprlist ')'
        ;

assignment:     expression '=' expression
        |       TOK_DEFAULT expression '=' expression
        ;

exprlist:       expression
        |       exprlist ',' expression
        ;

exprlist0:      /*              empty */
        |       exprlist
        ;

expression:     literal
        |       '{' exprlist0 '}'
        |       TOK_ID
        |       '.'
        |       fieldsel
        |       expression fieldsel
        |       expression '[' ']'
        |       expression '[' aggregate ']'
        |       expression '[' rangesel ']'
        |       '+' expression %prec TOK_UMINUS
        |       '-' expression %prec TOK_UMINUS
        |       '*' expression %prec TOK_UMINUS
        |       '/' expression 
        |       '%' expression %prec TOK_UMINUS
        |       '~' expression %prec TOK_UMINUS
        |       '#' expression
        |       sortop expression %prec TOK_UMINUS
        |       expression '+' expression
        |       expression '-' expression
        |       expression '*' expression
        |       expression '/' expression
        |       expression '%' expression
        |       expression '&' expression
        |       expression '^' expression
        |       expression '@' expression
        |       expression sortop expression %prec '$'
        |       expression TOK_MIN expression
        |       expression TOK_MAX expression
        |       expression TOK_MIN_CI expression
        |       expression TOK_MAX_CI expression
        ;

aggregate:      '+'
        |       '*'
        |       '&'
        |       '/'
        |       TOK_MIN
        |       TOK_MAX
        |       TOK_MIN_CI
        |       TOK_MAX_CI
        ;


fieldsel:       TOK_FIELD
        |       TOK_VARFIELD ']'
        |       TOK_VARFIELD '#' ']'
        |       TOK_VARFIELD expression ']'
        ;

rangesel:        expression
        |       expression ':' expression
        |       expression ':'
        |       ':' expression
        ;

literal:        TOK_STRING
        |       TOK_BINARY
        |       TOK_NUMBER
        |       TOK_TIMESTAMP
        ;

relop:          TOK_EQ
        |       TOK_NE
        |       TOK_EQ_CI
        |       TOK_NE_CI
        |       '~'
        |       TOK_NOT_MATCH
        |       TOK_MATCH_CI
        |       TOK_NOT_MATCH_CI
        |       orderop
        ;

orderop:              '<'
        |       '>'
        |       TOK_LE
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
