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
%token TOK_BINARY

%token TOK_ID
%token TOK_WILDCARD
%token TOK_RUNTIME

%token TOK_TRUE
%token TOK_FALSE
%token TOK_NULL
%token TOK_QUEUE
%token TOK_ME
%token TOK_END                        
                        
%token TOK_MODULE
%token TOK_LOCAL
%token TOK_PROTECTED
%token TOK_PUBLIC
%token TOK_IMPORT
%token TOK_PARTITION
%token TOK_PRAGMA
%token TOK_INCLUDE
%token TOK_AUGMENT


%left ';'                      
%left ','
%nonassoc TOK_RULE
%nonassoc TOK_FOREIGN                        
%nonassoc '=' TOK_ASSIGN_ONCE
%right TOK_THEN                        
%right TOK_HOT TOK_COLD TOK_IDLE TOK_GREEDY TOK_PASSIVE
%nonassoc TOK_ELSE
%right TOK_IF TOK_FOR TOK_FOREACH TOK_WHILE TOK_SWITCH TOK_TRY TOK_TYPECASE TOK_FREEZE TOK_WATCH TOK_WITH TOK_LET
%nonassoc TOK_KILL TOK_SUSPEND TOK_RESUME TOK_YIELD TOK_RECEIVE TOK_ERROR TOK_GOTO TOK_ASSERT TOK_NEED
%right TOK_PUT TOK_PUT_ALL TOK_PUT_NEXT 
%left TOK_PUT_BACK
%right '?'
%nonassoc TOK_MATCH_BINDING TOK_MATCH_BINDING_ALL
%left '|' '^'
%left '&'
%nonassoc TOK_EQ TOK_NE '<' '>' TOK_LE TOK_GE '~' TOK_NOT_MATCH TOK_IN TOK_ISTYPE
%right ':'                        
%left TOK_MAX TOK_MIN 
%left '+' '-' TOK_APPEND TOK_CHOP TOK_CHOP_HEAD
%left '*' '/' TOK_DIV TOK_MOD TOK_INTERSPERSE TOK_SPLIT '$' TOK_SUBST_ALL
%left TOK_TYPECAST
%left '[' 
%right '!' TOK_TRACING TOK_PEEK TOK_UMINUS TOK_TYPEOF
%left '.'
%nonassoc TOK_HOOK
%nonassoc TOK_ID
%{
extern int yylex(YYSTYPE* yylval_param, YYLTYPE * yylloc_param, exec_context *context);
static void yyerror(YYLTYPE * yylloc_param, exec_context *context, const char *msg);
%}

%%

script:         body
                ;

body:           /*empty*/
        |       body ';'
        |       body include ';'
        |       body module
        |       body pragma ';'
        |       body toplevelconditional 
                ;

pragma:         TOK_PRAGMA TOK_ID literal
                ;


module:         TOK_MODULE TOK_ID version0 '{' modulecontents '}'
                ;

include:        TOK_INCLUDE TOK_STRING 
        ;

version0:       /*empty*/
        |       version
                ;

version:        TOK_FLOAT
        |       TOK_TIMESTAMP
                ;

toplevelconditional: 
                TOK_IF '(' expression ')' '{' body '}' toplevelelse
                ;

toplevelelse:   /*empty*/
        |       TOK_ELSE '{' body '}'
                ;

versionconstraint: /*empty*/
        |       TOK_EQ version
        |       '>' version
        |       '<' version
        |       TOK_LE version
        |       TOK_GE version
        |       '~' version
                ;

modulecontents:  /*empty*/
        |       modulecontents ';'
        |       modulecontents include ';'
        |       modulecontents errordef ';'
        |       modulecontents pragma ';'
        |       modulecontents moduleconditional
        |       modulecontents declscope declaration
        |       modulecontents augment
        |       modulecontents moduleforeign ';'
                ;

moduleconditional: 
                TOK_IF '(' expression ')' '{' modulecontents '}' moduleelse
                ;

moduleelse:   /*empty*/
        |       TOK_ELSE '{' modulecontents '}'
                ;

moduleforeign: TOK_FOREIGN TOK_STRING
                ;

errordef:       TOK_ERROR TOK_ID '=' TOK_STRING
        ;

declscope:          /*empty*/
        |       TOK_PUBLIC
        |       TOK_PROTECTED
        |       TOK_LOCAL
                ;

declaration:    import ';'
        |       nodedecl
                ;

import:         TOK_IMPORT importpath versionconstraint importlist importalias
                ;

importpath:     TOK_ID
        |       importpath '/' TOK_ID
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

importalias:    /*empty*/
        |       TOK_TYPECAST TOK_ID
                ;

nodedecl:       nodepriority nodedecl1
                ;

nodedecl1:      TOK_ID nodedef
        |       TOK_PARTITION TOK_ID partition_channels '{' modulecontents '}'
                ;


nodepriority:   /*empty */
        |       TOK_IDLE
        |       hot_priority
        |       cold_priority
        |       TOK_GREEDY
        |       TOK_PASSIVE
        ;

hot_priority:   TOK_HOT
        |       hot_priority TOK_HOT
        ;

cold_priority:  TOK_COLD
        |       cold_priority TOK_COLD
        ;

partition_channels: '(' idlist ')'
        |       '(' idlist0 TOK_RULE idlist0 ')'
        ;

nodedef:        instantiate ';'
        |       nodeblock
                ;

instantiate:    '=' noderef '(' instanceargs0 ')' local_augments ';'
                ;

instanceargs0:  /*empty*/
        |       instanceargs
                ;

instanceargs:   instancearg
        |       instanceargs ',' instancearg
        ;

instancearg:    TOK_ID '=' expression
                ;

nodeblock:   '{' sequence  '}'
        |    '{' states '}'
                ;

states:         state
        |       states state
        ;

state:          statelabel TOK_RULE sequence
        ;

statelabel:     TOK_ID
        |       TOK_ERROR
        |       TOK_KILL
        ;

augment:        TOK_AUGMENT hookref block
        ;

noderef:        TOK_ID
        |       TOK_ID '.' TOK_ID %prec '.'
                ;

hookref:        TOK_ID '.' TOK_ID  %prec '.'
        |       TOK_ID '.' TOK_ID '.' TOK_ID %prec '.' 
                ;

expression: literal
        |       '[' exprlist0 ']'
        |       '[' assoclist ']' 
        |       '(' expression ')' 
        |       '.' assockey
        |       block
        |       anonymous_node
        |       TOK_ID
        |       TOK_WILDCARD
        |       TOK_ID '(' exprlist0 ')'
        |       TOK_HOOK TOK_ID
        |       TOK_ME
        |       TOK_RUNTIME
        |       TOK_QUEUE
        |       TOK_REGEXP
        |       expression '.' assockey                
        |       expression '[' expression0 ']'
        |       '+' expression %prec TOK_UMINUS 
        |       '-' expression %prec TOK_UMINUS 
        |       '*' expression %prec TOK_UMINUS 
        |       '^' expression %prec TOK_UMINUS
        |       TOK_TRACING expression
        |       '?' expression  %prec TOK_UMINUS
        |       '&' expression  %prec TOK_UMINUS
        |       '~' pattern  %prec TOK_UMINUS
        |       TOK_TYPEOF expression
        |       '!' expression
        |       TOK_PEEK expression
        |       expression '+' expression 
        |       expression '-' expression 
        |       expression '*' expression 
        |       expression '/' expression 
        |       expression TOK_DIV expression
        |       expression TOK_MOD expression
        |       expression '^' expression 
        |       expression '&' expression
        |       expression '|' expression
        |       expression '?' expression
        |       expression ':' expression
        |       expression TOK_ELSE expression
        |       expression TOK_MIN expression 
        |       expression TOK_MAX expression
        |       expression TOK_APPEND expression
        |       expression TOK_INTERSPERSE expression
        |       expression TOK_CHOP expression
        |       expression TOK_CHOP_HEAD expression
        |       expression '$' expression
        |       expression TOK_SUBST_ALL expression
        |       expression TOK_SPLIT expression
        |       expression TOK_TYPECAST typename
        |       expression TOK_THEN expression                
        |       expression TOK_EQ expression
        |       expression TOK_NE expression
        |       expression '<' expression
        |       expression '>' expression
        |       expression TOK_LE expression
        |       expression TOK_GE expression
        |       expression '~' pattern
        |       expression TOK_MATCH_BINDING pattern
        |       expression TOK_MATCH_BINDING_ALL pattern                                
        |       expression TOK_NOT_MATCH pattern
        |       expression TOK_IN expression
        |       expression TOK_ISTYPE typename
        |       expression '=' expression
        |       expression TOK_PUT expression
        |       expression TOK_PUT_ALL expression
        |       expression TOK_PUT_NEXT expression
        |       expression TOK_PUT_BACK expression                
        |       TOK_KILL expression
        |       TOK_SUSPEND expression
        |       TOK_RESUME expression
        |       TOK_YIELD expression
        |       TOK_RECEIVE expression                
        |       TOK_ERROR expression
        |       TOK_ASSERT expression
        |       TOK_NEED expression
        |       TOK_GOTO gotodest
        |       TOK_IF '(' expression ')' expression %prec TOK_IF
        |       TOK_WHILE '(' expression ')' expression %prec TOK_WHILE
        |       TOK_FOR '(' bindings ';' expression ';' bindings ')' expression %prec TOK_FOR
        |       TOK_FOREACH '(' foreach_spec ')' expression %prec TOK_FOREACH
        |       TOK_FREEZE '(' expression ')' expression %prec TOK_FREEZE
        |       TOK_SWITCH '(' expression ')' '{' alternatives '}' %prec TOK_SWITCH
        |       TOK_TRY '{' try_alternatives '}' %prec TOK_TRY
        |       TOK_TYPECASE '(' expression ')' '{' generic_alternatives '}' %prec TOK_TYPECASE
        |       TOK_WATCH '(' expression TOK_RULE expression ')' expression %prec TOK_WATCH
        |       TOK_WITH  '(' expression ')' expression %prec TOK_WITH
        |       TOK_LET '(' bindings ')' expression %prec TOK_LET
        |       TOK_HOT expression
        |       TOK_COLD expression
        |       TOK_IDLE expression
        |       TOK_GREEDY expression
        |       TOK_FOREIGN  TOK_ID '(' exprlist0 ')' TOK_STRING %prec TOK_FOREIGN
                ;

gotodest:       TOK_ID
        |       TOK_WILDCARD
        |       TOK_PUT_BACK
        ;

anonymous_node: '@' noderef block local_augments
        |       '@' noderef '(' callexpr ')'
        |       '@' TOK_FOR  '(' bindings ';' expression0 ';' bindings ')' expression %prec TOK_FOR
        |       '@' TOK_FOREACH '(' foreach_spec ')' expression %prec TOK_FOREACH
                ;

callexpr:       expression
        |       exprlist ',' expression
        |       assoclist
        ;

local_augments: /* empty */
        |       local_augments TOK_AUGMENT TOK_ID block

expression0:    /*empty*/
        |       expression
        ;

bindings:       binding 
        |       bindings ';' binding
                ;

binding: binding_scope TOK_ID assign_op expression %prec '='
        | pragma
                ;

assign_op:      '='
        |       TOK_ASSIGN_ONCE
        ;

binding_scope:  TOK_LOCAL
        |       TOK_PUBLIC
        |       TOK_PROTECTED
        |       TOK_MODULE TOK_PUBLIC
        ;

block:   '{'    sequence '}'
                ;

sequence:       expression ';'
        |       bindings ';'
        |       sequence expression ';'
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

assoc:          xassockey TOK_RULE expression
                ;

xassockey:      assockey
        |       '~' pattern
                ;

assockey:       TOK_ID
        |       TOK_REGEXP
        |       literal
        |       block
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

alternative:    patternseq TOK_RULE expression
        ;

patternseq:     lastpattern
        |       cpatternseq
        |       cpatternseq ',' lastpattern
        |       TOK_ELSE
                ;

lastpattern: '?' guarded_pattern
        ;

cpatternseq:    guarded_pattern
        |       cpatternseq ',' guarded_pattern
                ;

guarded_pattern:pattern
        |       pattern TOK_IF '(' expression ')'
                
try_alternatives:
                try_alternative
        |       try_alternatives ';' try_alternative
        ;

try_alternative:    expression TOK_RULE expression
        |       TOK_ELSE TOK_RULE expression
        ;


generic_alternatives:
                generic_alternative
        |       generic_alternatives ';' generic_alternative
                ;

generic_alternative: typechoice TOK_RULE expression
        |       TOK_ELSE TOK_RULE expression
        ;

typechoice:     typename
        |       typechoice '|' typename %prec '.'
        ;

typename:       TOK_ID
        |       TOK_NULL
        |       TOK_END
        |       TOK_TYPEOF expression
        ;

pattern:        TOK_ID
        |       TOK_WILDCARD
        |       literal
        |       TOK_REGEXP
        |       '(' pattern ')'
        |       block                
        |       TOK_ID '=' pattern %prec TOK_EQ
        |       TOK_ID '(' patternlist0 ')' %prec '('
        |       '[' patternlist ']'
        |       '[' patternassoclist ']'
        |       pattern ':' pattern
        |       pattern '|' pattern
        ;

patternlist0: /*empty*/
        |       patternlist
        ;

patternlist:    pattern
        |       patternlist ',' pattern
        ;

patternassoclist:   patternassoc
        |       patternassoclist ',' patternassoc
        ;

patternassoc:   passockey TOK_RULE pattern
        ;

passockey:      assockey
        |       TOK_WILDCARD
        ;

literal:        TOK_STRING
        |       TOK_INTEGER
        |       TOK_CHARACTER
        |       TOK_BINARY
        |       TOK_FLOAT
        |       TOK_TIMESTAMP
        |       TOK_NULL
        |       TOK_TRUE
        |       TOK_FALSE
        |       TOK_END
                ;

%%


static void yyerror(YYLTYPE * yylloc_param, exec_context *context ATTR_UNUSED, const char *msg)
{
}
