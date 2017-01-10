%define api.pure full
%locations
%error-verbose

%{
#include <stdio.h>

%}

%token TOK_CHARACTER
%token TOK_STRING
%token TOK_INTEGER
%token TOK_FLOAT
%token TOK_TIMESTAMP
%token TOK_REGEXP

%token TOK_TRUE
%token TOK_FALSE
                        
%token TOK_EXTERN
%token TOK_IMPORT
%token TOK_PRAGMA
%token TOK_TYPE

%nonassoc '='
%nonassoc TOK_MAP 
%left TOK_ALT
%nonassoc ':'                                                                       
%precedence TOK_IF
%token TOK_SWITCH TOK_ELSE
%precedence TOK_LOOP
%right TOK_OUTGOING                        
%left TOK_INCOMING
%left TOK_INTERACT
%right TOK_ARROW
%left '|'
%left '&'
%nonassoc TOK_EQ TOK_NE '<' '>' TOK_LE TOK_GE '~' TOK_NOT_MATCH
%precedence TOK_RANGE                        
%nonassoc '#'                       
%left TOK_MAX TOK_MIN
%left '+' '-' TOK_APPEND
%left '*' '/' '%'                        
%precedence TOK_NEW
%precedence '!' TOK_UMINUS '^'
%precedence '.'
%precedence '[' '(' '{'
%token TOK_ID
%{
extern int yylex(YYSTYPE* yylval_param, YYLTYPE * yylloc_param, exec_context *context);
static void yyerror(YYLTYPE * yylloc_param, exec_context *context, const char *msg);
%}

%%

script:         body
                ;

body:           %empty
        |       body body_item ';' 
        ;

body_item:      definition
        |       expression
        |       import
        |       pragma
        ;


pragma:         TOK_PRAGMA TOK_ID literal
                ;

import:         TOK_IMPORT TOK_ID importlist0
                ;

importlist0:     %empty
        |       '(' importlist ')'
                ;

importlist:     import_item
        |       importlist ',' import_item
        ;

import_item:    TOK_ID
        |       TOK_ID TOK_MAP TOK_ID
        ;


definition:     override0 TOK_ID def_args '=' expression
        ;


override0:      %empty
        |       TOK_OVERRIDE
        ;


def_args:       %empty
        |       '(' lambda_args ')'
        ;

comparison:     TOK_EQ
        |       TOK_NE
        |       '<'
        |       '>'
        |       TOK_LE
        |       TOK_GE
|
        ;

label:          TOK_ID '='
        ;

expression:      literal
        |       TOK_ID
        |       '\\' lambda_args TOK_MAP expression
        |       '~' expression %prec TOK_UMINUS
        |       '?' expression %prec TOK_UMINUS
        |       expression '(' application ')'
        |       '(' expression ')'
        |       '[' expr_list0 ']'
        |       '{' body '}'
        |       '(' expression ',' expression ')'
        |       '+' expression %prec TOK_UMINUS
        |       '-' expression %prec TOK_UMINUS
        |       '^' expression
        |       '#' expression %prec TOK_UMINUS
        |       '!' expression
        |       '*' expression %prec TOK_UMINUS
        |       TOK_TYPEOF expression
        |       TOK_MIN expression %prec TOK_UMINUS
        |       TOK_MAX expression %prec TOK_UMINUS
        |       expression '+' expression
        |       expression '-' expression
        |       expression '*' expression
        |       expression '/' expression
        |       expression '%' expression
        |       expression '#' expression
        |       expression '&' expression
        |       expression '|' expression
        |       expression TOK_MIN expression
        |       expression TOK_MAX expression
        |       expression TOK_APPEND expression
        |       expression comparison expression %prec TOK_EQ
        |       expression '~' expression
        |       expression TOK_NOT_MATCH expression
        |       expression '[' expression ']'  %prec '['
        |       expression TOK_ARROW expression
        |       expression TOK_INTERACT expression
        |       expression '.' TOK_ID
        |       TOK_ID '^' expression
        |       TOK_IF '(' expression ')' expression TOK_ELSE expression %prec TOK_IF
        |       TOK_LOOP expression
        |       TOK_SWITCH '(' expression ')' '{' switch_branches '}' %prec TOK_SWITCH
        |       TOK_NEW expression
        |       expression ':' expression
        |       expression '?' expression
        ;

lambda_args:    lambda_arg
        |       lambda_args ',' lambda_arg
        ;

lambda_arg:     override0 TOK_ID defval0
        ;

defval0:        %empty
        |       '=' expression
        ;

expr_list:      %empty
        |       expr_list1
        ;

expr_list1:     expression
        |       expr_list1 ',' expression
        ;

application:    named_args
        |       ordered_args
        ;

named_args:     named_app_arg
        |       named_args ',' named_app_arg
        ;

named_app_arg:  label expression
        |       TOK_TYPE label type_expr
        ;

ordered_args:   ordered_app_arg
        |       ordered_args ',' ordered_app_arg
        ;

ordered_app_arg: expression
        |       TOK_TYPE type_expr
        ;

range:          expression TOK_RANGE expression
        |       expression TOK_RANGE
        |       TOK_RANGE expression
        ;

switch_branches: switch_branch
        |        switch_branches switch_branch
        ;

switch_branch:  switch_tags '=' expression ';'
        ;

switch_tags:    TOK_ELSE
        |       switch_tags1
                ;

switch_tags1:   expression
        |       switch_tags1 ',' expression
        ;

type_switch_branches: type_switch_branch
        |       type_switch_branches type_switch_branch
        ;

type_switch_branch: type_switch_tags '=' expression ';'
        ;

type_switch_tags: TOK_ELSE
        |       type_switch_tags1
        ;

type_switch_tags1: type_expr
        |       type_switch_tags1 ',' type_expr
        ;

literal:        TOK_STRING
        |       TOK_INTEGER
        |       TOK_FLOAT
        |       TOK_CHARACTER
        |       TOK_TIMESTAMP
        |       TOK_TRUE
        |       TOK_FALSE
                ;

regexp_match:   '~' TOK_REGEXP
        |       TOK_NOT_MATCH TOK_REGEXP
        ;

%%


static void yyerror(YYLTYPE * yylloc_param, exec_context *context ATTR_UNUSED, const char *msg)
{
}
