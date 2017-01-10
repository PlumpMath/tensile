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
%token TOK_OVERRIDE                        

%token TOK_ID
                        
%token TOK_ALT

%token TOK_LDBRACKET TOK_RDBRACKET TOK_LDBRACE TOK_RDBRACE
                                                    
%precedence TOK_MAP
%nonassoc ':'
%left '?'                                                                                               
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
%nonassoc TOK_RANGE                        
%nonassoc '#'                       
%left TOK_MAX TOK_MIN
%left '+' '-' TOK_APPEND
%left '*' '/' '%'                        
%precedence '!' TOK_UMINUS '^' TOK_NEW TOK_TYPEOF
%precedence '.'
%precedence '(' '[' 
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
        |       '~'
        |       TOK_NOT_MATCH
        ;

expression:      literal
        |       TOK_ID
        |       TOK_EXTERN string0 TOK_ID
        |       '\\' lambda_args TOK_MAP expression
        |       '~' expression %prec TOK_UMINUS
        |       '?' expression %prec TOK_UMINUS
        |       expression '(' application ')'
        |       '(' expression ')'
        |       '[' expr_list ']'
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
        |       expression '[' expression ']'
        |       expression '[' renaming ']'
        |       expression TOK_LDBRACKET algebraic_type TOK_RDBRACKET %prec '['
        |       TOK_LDBRACKET algebraic_type TOK_RDBRACKET %prec '['
        |       TOK_LDBRACE signature TOK_RDBRACE %prec '['
        |       expression TOK_ARROW expression
        |       expression TOK_INTERACT expression
        |       expression TOK_INCOMING expression
        |       expression TOK_OUTGOING expression
        |       expression TOK_RANGE expression
        |       TOK_RANGE expression
        |       expression TOK_RANGE
        |       expression '.' TOK_ID
        |       TOK_ID '^' expression
        |       TOK_IF '(' expression ')' expression TOK_ELSE expression %prec TOK_IF
        |       TOK_LOOP expression
        |       TOK_SWITCH '(' expr_list1 ')' '{' switch_branches '}' %prec TOK_SWITCH
        |       TOK_NEW expression
        |       expression ':' expression
        |       expression '?' expression
        ;

string0:        %empty
        |       TOK_STRING
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

application:    app_arg
        |       application ',' app_arg
        ;

app_arg:        expression
        |       TOK_ID '=' expression
        ;

switch_branches: switch_branch
        |        switch_branches switch_branch
        ;

switch_branch:  switch_tags TOK_MAP expression ';'
        ;

switch_tags:    TOK_ELSE
        |       expr_list1
                ;

renaming:       renaming_item
        |       renaming ',' renaming_item
        ;

renaming_item:  port_name TOK_MAP TOK_ID
        ;

port_name:      TOK_ID
        |       TOK_ID '^'
        ;

algebraic_type: algebraic_branch
        |       algebraic_type TOK_ALT algebraic_branch
        ;

algebraic_branch: algebraic_field
        |       algebraic_branch ',' algebraic_field
        ;

algebraic_field: TOK_ID ':' expression defval0
        ;

signature:      sig_field
        |       signature ',' sig_field
        ;

sig_field:      port_name ':' expression
        ;

literal:        TOK_STRING
        |       TOK_INTEGER
        |       TOK_FLOAT
        |       TOK_CHARACTER
        |       TOK_TIMESTAMP
        |       TOK_TRUE
        |       TOK_FALSE
        |       TOK_REGEXP
                ;


%%


static void yyerror(YYLTYPE * yylloc_param, exec_context *context ATTR_UNUSED, const char *msg)
{
}
