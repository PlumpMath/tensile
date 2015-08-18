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

%token TOK_ID
%token TOK_WILDCARD                        

%token TOK_TRUE
%token TOK_FALSE
%token TOK_NULL
%token TOK_QUEUE
%token TOK_ME
                        
%token TOK_MODULE
%token TOK_EXTERN
%token TOK_LOCAL
%token TOK_IMPORT
%token TOK_PROTOTYPE
%token TOK_FOREIGN
%token TOK_PRAGMA                        

                                                                                                
%left ';'                      
%left ','
%nonassoc TOK_RULE                        
%nonassoc TOK_ASSIGN
%nonassoc TOK_SEQ
%nonassoc TOK_ELSE                                                
%nonassoc TOK_IF TOK_FOR TOK_FOREACH TOK_WHILE TOK_SWITCH TOK_GENERIC TOK_SELECT TOK_WITH TOK_FREEZE TOK_GOTO
%nonassoc TOK_KILL TOK_SUSPEND TOK_RESUME TOK_YIELD TOK_ERROR
%right TOK_PUT TOK_PUT_ALL TOK_PUT_NEXT 
%left '='                        
%nonassoc TOK_MATCH_BINDING
%nonassoc TOK_EQ TOK_NE '<' '>' TOK_LE TOK_GE '~' TOK_NOT_MATCH TOK_IN TOK_ISTYPE
%left '?'
%left '|' '^'
%left '&'
%left TOK_MAX TOK_MIN 
%left '+' '-' TOK_APPEND TOK_CHOP '#'
%left '*' '/' TOK_DIV TOK_MOD TOK_INTERSPERSE TOK_SPLIT
%nonassoc TOK_TYPECAST                        
%right '!' TOK_PEEK TOK_UMINUS
%left '.'                        

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

versionconstraint: /*empty*/
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
        |       modulecontents pragma ';'
        |       modulecontents moduleconditional
        |       modulecontents scope declaration 
                ;

moduleconditional: 
                TOK_IF '(' tlmcondition ')' '{' modulecontents '}' moduleelse
                ;

moduleelse:   /*empty*/
        |       TOK_ELSE '{' modulecontents '}'
                ;


scope:          /*empty*/
        |       TOK_EXTERN
        |       TOK_LOCAL
                ;

declaration:    foreign ';'
        |       import ';'
        |       nodedecl
                ;

import:         TOK_IMPORT importname versionconstraint importlist 
                ;

importlist:     /*empty*/
        |       '(' idlist0 ')'
                ;

idlist0:        /*empty*/
        |       idlist
                ;

idlist:         TOK_ID
        |       idlist ',' TOK_ID
                ;

importname:     TOK_ID
        |       TOK_ID TOK_ASSIGN TOK_ID
                ;

foreign:  TOK_FOREIGN TOK_ID TOK_ASSIGN TOK_STRING foreignsym
                ;

foreignsym:     /*empty*/
        |        '.' TOK_ID
                ;

nodedecl:       prototype0 TOK_ID '(' nodeargs ')' nodedef
                ;

prototype0:     /*empty*/
        |       TOK_PROTOTYPE
        ;

nodedef:        instantiate ';'
        |       nodeblock
                ;

nodeargs:       nodearg
        |       nodeargs ',' nodearg
                ;

nodearg:        nodeargname
        |       nodeargname TOK_ASSIGN expression
                ;

nodeargname:    scope TOK_ID
        ;

instantiate:    TOK_ASSIGN noderef '(' instanceargs0 ')' ';'
                ;

instanceargs0:  /*empty*/
        |       instanceargs
                ;

instanceargs:   instancearg
        |       instanceargs ',' instancearg
        ;

instancearg:    TOK_ID TOK_ASSIGN expression
                ;

nodeblock:   '{' bindings sequence '}'
        |    '{' stateful '}'
        |    '{'sequence '}'
                ;

stateful:       state
        |       stateful state
        ;

state:          TOK_ID ':' stateblock
        |       TOK_ERROR ':' stateblock
        |       TOK_KILL ':' stateblock
        ;

stateblock:     bindings sequence
        |       sequence
        ;

noderef:        TOK_ID
        |       TOK_ID '.' TOK_ID
                ;

varref:         TOK_ID
        |       TOK_ID '.' TOK_ID
        |       TOK_ID '.' TOK_ID '.' TOK_ID
        ;

simple_expression:     literal 
        |       '[' exprlist0 ']' 
        |       '[' assoclist ']'
        |       '(' expression ')' 
        |       varref
        |       block
        |       anonymous_node
        |       TOK_ID '(' exprlist0 ')' 
        |       TOK_ME
        |       TOK_QUEUE
        |       TOK_REGEXP
        |       simple_expression '[' ']' 
        |       simple_expression '[' expression ']' 
        |       '+' simple_expression %prec TOK_UMINUS 
        |       '-' simple_expression %prec TOK_UMINUS 
        |       '*' simple_expression %prec TOK_UMINUS 
        |       '^' simple_expression %prec TOK_UMINUS
        |       '#' simple_expression %prec TOK_UMINUS
        |       '?' simple_expression %prec TOK_UMINUS
        |       '!' simple_expression
        |       TOK_PEEK simple_expression
        |       simple_expression '+' simple_expression 
        |       simple_expression '-' simple_expression 
        |       simple_expression '*' simple_expression 
        |       simple_expression '/' simple_expression 
        |       simple_expression TOK_DIV simple_expression
        |       simple_expression TOK_MOD simple_expression
        |       simple_expression '&' simple_expression 
        |       simple_expression '^' simple_expression 
        |       simple_expression '|' simple_expression 
        |       simple_expression '?' simple_expression
        |       simple_expression '#' simple_expression
        |       simple_expression TOK_MIN simple_expression 
        |       simple_expression TOK_MAX simple_expression
        |       simple_expression TOK_APPEND simple_expression
        |       simple_expression TOK_INTERSPERSE simple_expression
        |       simple_expression TOK_CHOP simple_expression
        |       simple_expression TOK_SPLIT simple_expression
        |       simple_expression TOK_TYPECAST TOK_ID
        |       simple_expression TOK_SEQ simple_expression
        |       simple_expression TOK_EQ simple_expression
        |       simple_expression TOK_NE simple_expression
        |       simple_expression '<' simple_expression
        |       simple_expression '>' simple_expression
        |       simple_expression TOK_LE simple_expression
        |       simple_expression TOK_GE simple_expression
        |       simple_expression '~' pattern
        |       simple_expression TOK_MATCH_BINDING pattern
        |       simple_expression TOK_NOT_MATCH pattern
        |       simple_expression TOK_IN simple_expression
        |       simple_expression TOK_ISTYPE typechoice
        |       simple_expression '=' simple_expression
        |       simple_expression TOK_PUT simple_expression
        |       simple_expression TOK_PUT_ALL simple_expression
        |       simple_expression TOK_PUT_NEXT simple_expression
        |       TOK_KILL simple_expression
        |       TOK_SUSPEND simple_expression
        |       TOK_RESUME simple_expression
        |       TOK_YIELD simple_expression
        |       TOK_ERROR simple_expression
        |       TOK_GOTO TOK_ID
                ;
                
expression:     simple_expression
        |       TOK_IF '(' expression ')' simple_expression 
        |       TOK_IF '(' expression ')' simple_expression TOK_ELSE simple_expression                
        |       TOK_WHILE '(' expression ')' simple_expression
        |       TOK_FOR '(' bindings ';' expression ';' bindings ')' simple_expression
        |       TOK_FOREACH '(' foreach_spec ')' simple_expression
        |       TOK_WITH '(' TOK_ID ')' simple_expression
        |       TOK_FREEZE '(' expression ')' simple_expression
        |       TOK_SWITCH '(' expression ')' '{' alternatives '}'
        |       TOK_SELECT '{' select_alternatives '}'
        |       TOK_GENERIC '(' expression ')' '{' generic_alternatives '}'
                ;

anonymous_node: '@' noderef '(' bindings0 ')'
        ;

bindings:       binding 
        |       bindings ';' binding
                ;

bindings0:     /*empty*/
        |      bindings
        ;

binding: TOK_ID TOK_ASSIGN expression
                ;

block:   '{'    sequence '}'
        |       '{' bindings sequence '}'
                ;

sequence:       expression ';'
        |       sequence expression
                ;

exprlist0:      /*empty*/
        |       exprlist
                ;

exprlist:       expression
        |       exprlist ',' expression
                ;

assoclist:      assoc
        |       assoclist ',' assoc
                ;

assoc:          TOK_ID TOK_RULE expression
                ;

foreach_spec:   generators foreach_dest
        ;

generators:     generator
        |       generators ';' generator
        ;

generator:      TOK_ID TOK_IN expression
        ;

foreach_dest:   /*empty*/
        |       TOK_RULE TOK_ID
        ;

alternatives:   alternative
        |       alternatives ';' alternative
        ;

alternative:    pattern TOK_RULE expression
        |       pattern TOK_IF '(' expression ')' TOK_RULE expression
        |       TOK_ELSE TOK_RULE expression
        ;

select_alternatives:
                select_alternative
        |       select_alternatives ';' select_alternative
        ;

select_alternative:    selector TOK_RULE expression
        |       TOK_ELSE TOK_RULE expression
        ;

selector:       varref
        |       varref '[' expression ']'
        |       '!' varref
        |       '^' varref 
        |       '(' selector ')'
        |       selector '&' selector
        |       selector '|' selector
        |       selector '^' selector
        ;

generic_alternatives:
                generic_alternative
        |       generic_alternatives ';' generic_alternative
                ;

generic_alternative: typechoice TOK_RULE expression
        |       TOK_ELSE TOK_RULE expression
        ;

typechoice:     TOK_ID
        |       typechoice '|' TOK_ID
        ;

pattern:        TOK_ID
        |       TOK_WILDCARD
        |       literal
        |       TOK_REGEXP
        |       '(' expression ')'
        |       TOK_ID TOK_ASSIGN pattern
        |       TOK_ID '(' patternlist0 ')' 
        |       '[' patternlist ']'
        ;

patternlist0: /*empty*/
        |       patternlist
        ;

patternlist:    pattern
        |       patternlist ',' pattern
        ;

literal:        TOK_STRING
        |       TOK_INTEGER
        |       TOK_CHARACTER
        |       TOK_FLOAT
        |       TOK_TIMESTAMP
        |       TOK_NULL
        |       TOK_TRUE
        |       TOK_FALSE
                ;

%%


static void yyerror(YYLTYPE * yylloc_param, exec_context *context ATTR_UNUSED, const char *msg)
{
}
