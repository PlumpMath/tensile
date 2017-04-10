/**************************************************************************
 * Copyright (c) 2017 Artem V. Andreev
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
**************************************************************************/

%define api.pure full
%locations
%error-verbose

%{
#include <stdio.h>

%}

%token TOK_CHARACTER
%token TOK_STRING
%token TOK_BITSTRING                        
%token TOK_INTEGER
%token TOK_FLOAT
%token TOK_TIMESTAMP
%token TOK_REGEXP

%token TOK_TRUE
%token TOK_FALSE
                        
%token TOK_EXTERN
%token TOK_IMPORT
%token TOK_PRAGMA

%precedence TOK_MAP
%nonassoc ':'
%left '?'                                                                                               
%precedence TOK_IF
%token TOK_SWITCH TOK_ELSE
%precedence TOK_LOOP
%left TOK_OUTGOING                        
%right TOK_INCOMING
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
%precedence '!' TOK_UMINUS
%precedence '[' '{'
%precedence '.'
%precedence TOK_ID
%precedence '('                        
%{
extern int yylex(YYSTYPE* yylval_param, YYLTYPE * yylloc_param, exec_context *context);
static void yyerror(YYLTYPE * yylloc_param, exec_context *context, const char *msg);
%}

%%

module:         declarations
                ;

declarations:           %empty
        |       declarations declaration ';' 
        ;

declaration:    entity_decl
        |       extern_decl
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
        |       TOK_ID '=' TOK_ID
        ;

extern_decl:    TOK_EXTERN library_name extern_entity_decl
        ;


library_name:   %empty
        |       TOK_STRING
        ;

extern_entity_decl: object_decl
        |       type_decl
        ;

entity_decl:     object_decl
        |       TOK_STATIC object_decl
        |       type_decl
        |       struct_decl
        |       union_decl
        |       enum_decl
        ;

object_decl:    type_expression TOK_ID object_def0
        ;

object_def0:    %empty
        |       initializer
        |       function
        ;

initializer:    '=' expression
        ;

function:       '(' function_args ')' function_body0
        ;

function_args:  function_arg
        |       function_args ',' function_arg
        ;

function_arg:   type_expression TOK_ID
        ;

function_body0: %empty
        |       TOK_GENERIC '(' idlist ')'
        |       block
        ;

type_decl:      TOK_TYPEDEF TOK_ID typedef0
        ;

typedef0:       %empty
        |       typedef
        ;

typedef: '=' typedef_expression type_constraint
        ;


typedef_expression: type_expression
        |       typedef_expression '&' TOK_ID
        ;

type_constraint: %empty
        |       TOK_IF expression
        ;

struct_decl:    TOK_STRUCT TOK_ID compound_body
        ;

union_decl:     TOK_UNION TOK_ID compound_body
        ;

compound_body: %empty
        |       '{' compound_fields '}'
        ;

compound_fields: compound_field
        |      compound_fields compound_field ';' 
        ;

compound_field: type_expression TOK_ID
        ;

enum_decl:      TOK_ENUM TOK_ID enum_body
        ;

enum_body:      %empty
        |       '{' idlist '}'
        ;

idlist:         TOK_ID
        |       idlist ',' TOK_ID
        ;


type_expression: TOK_ID
        | '(' type_expression ')'
        |       type_expression '[' ']'
        |       type_expression TOK_LDBRACKET TOK_RDBRACKET
        |       type_expression '*'
        |       type_expression '?'
        |       type_expression TOK_RANGE
        |       type_expression TOK_MAP type_expression
        |       '*' type_expression
        |       '^' type_expression
        |       type_expression '^' type_expression
        |       '(' tuple_type ')'
        |       type_expression '(' type_list ')'
        ;

tuple_type:    type_expression ',' type_expression
        |       tuple_type ',' type_expression
        ;

type_list:      type_expression
        |       type_list ',' type_expression
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
        |       TOK_REGEXP
        |       TOK_ID
        |       '\\' function_args block
        |       '~' expression %prec TOK_UMINUS
        |       '?' expression %prec TOK_UMINUS
        |       expression '(' application ')' %prec '['
                '(' ')'
        |       '(' expression ')'
        |       '[' expr_list ']'
        |       '[' expr_map_list ']'
        |       TOK_LDBRACKET expr_list TOK_RDBRACKET
        |       block
        |       '(' tuple ')'
        |       '+' expression %prec TOK_UMINUS
        |       '-' expression %prec TOK_UMINUS
        |       '^' expression %prec TOK_UMINUS
        |       '!' expression
        |       '*' expression %prec TOK_UMINUS
        |       '&' expression %prec TOK_UMINUS
        |       TOK_SIZEOF expression
        |       TOK_APPEND expression %prec TOK_UMINUS
        |       expression '+' expression
        |       expression '-' expression
        |       expression '*' expression
        |       expression '/' expression
        |       expression '%' expression
        |       expression '&' expression
        |       expression '|' expression
        |       expression '^' expression
        |       expression TOK_APPEND expression
        |       expression TOK_DISTANCE expression
        |       expression TOK_MATCH_RESULT expression
        |       expression comparison expression %prec TOK_EQ
        |       expression '[' expression ']' %prec '['
        |       expression TOK_ARROW expression
        |       expression TOK_RANGE expression
        |       TOK_RANGE expression
        |       expression TOK_RANGE
        |       expression '.' TOK_ID %prec '.'
        |       TOK_IF '(' expression ')' expression TOK_ELSE expression %prec TOK_IF
        |       TOK_LOOP loop_expression
        |       TOK_WHILE '(' expression ')' loop_expression %prec TOK_WHILE
        |       TOK_FOREACH '(' foreach_spec ')' loop_expression %prec TOK_FOREACH
        |       TOK_FOR '(' local_var ';' expression ';' TOK_ID '=' expression ')' loop_expression %prec TOK_FOR
        |       TOK_SWITCH '(' expr_list1 ')' '{' switch_branches '}' %prec TOK_SWITCH
        |       TOK_GENERIC '(' expr_list1 ')' '{' generic_branches '}' %prec TOK_GENERIC
        |       TOK_DISPATCH '(' expr_list1 ')' '{' dispatch_branches '}' %prec TOK_DISPATCH
        |       TOK_SELECT '{' select_branches '}' %prec TOK_DISPATCH
        |       TOK_BREAK expression
        |       TOK_CONTINUE expression
        |       TOK_RETURN expression
        |       expression TOK_TYPECHECK type_expression
        |       expression '?' type_expression
        |       TOK_ASSERT expression
        |       TOK_ATEXIT atexit_arg expression
        ;

atexit_arg:     '(' TOK_ID ')'
        |       %empty
        ;

local_var:     type_expression TOK_ID '=' expression
        ;

foreach_spec:   foreach_var
        |       foreach_spec ';' foreach_var
        ;

foreach_var:    type_expression TOK_ID foreach_scope
        ;

foreach_scope:  %empty
        |       '~' expression
        ;

loop_expression: expression
        |       TOK_GATHER '(' local_var ')'
        ;

application:    expr_list1
        |       named_application
                ;


expr_list:      %empty
        |       expr_list1
        ;

expr_list1:     expression
        |       expr_list1 ',' expression
        ;

expr_map_list:  expr_map
        |       expr_map_list ',' expr_map
        ;

expr_map:       expression TOK_MAP expression
        ;

named_application: named_arg
        |       named_application ',' named_arg
        ;

named_arg:      TOK_ID TOK_MAP expression
        ;


switch_branches: switch_branch
        |       switch_branches switch_branch
        ;

switch_branch:  switch_cases expression semicolon0
        ;

switch_cases:   TOK_ELSE ':'
        |       switch_cases1
        ;

switch_cases1:  switch_case
        |       switch_cases1 switch_case
        ;

switch_case:    TOK_CASE expr_list1 ':'
        ;


generic_branches: generic_branch
        |       generic_branches generic_branch
        ;

generic_branch:  generic_cases expression semicolon0
        ;

generic_cases:   TOK_ELSE ':'
        |       generic_cases1
        ;

generic_cases1:  generic_case
        |       generic_cases1 generic_case
        ;

generic_case:    TOK_CASE generic_bindings ':'
        ;

generic_bindings: compound_field
        |       generic_bindings ',' compound_field
                ;


dispatch_branches dispatch_branch
        |       dispatch_branches dispatch_branch
        ;

dispatch_branch: dispatch_case expression semicolon0
        ;

dispatch_case:  TOK_ELSE ':'
        |       TOK_CASE dispatch_bindings ':'
        ;

dispatch_bindings: dispatch_binding
        |       dispatch_bindings ',' dispatch_binding
        ;

dispatch_binding: TOK_ID
        |       TOK_NOTHING
        |       TOK_TRUE
        |       TOK_FALSE
        |       '[' ']'
        ;

select_branches: select_branch
        |       select_branches select_branch
        ;

select_branch:  select_case expression semicolon0
        ;

select_case:    select_cases1
        |       TOK_ELSE ':'
        |       TOK_ATEXIT atexit_arg ':'
        ;

select_cases1:  TOK_CASE TOK_ID ':'
        |       select_cases1 TOK_CASE TOK_ID ':'
        ;

semicolon0:     %empty
        |       ';'
        ;

literal:        TOK_STRING
        |       TOK_INTEGER
        |       TOK_FLOAT
        |       TOK_CHARACTER
        |       TOK_TIMESTAMP
        |       TOK_BITSTRING
        |       TOK_TRUE
        |       TOK_FALSE
        |       TOK_NOTHING
        |       '(' ')'
                ;

%%


static void yyerror(YYLTYPE * yylloc_param, exec_context *context ATTR_UNUSED, const char *msg)
{
}
