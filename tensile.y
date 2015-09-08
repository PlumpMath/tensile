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
%token TOK_PARTITION
%token TOK_PRAGMA
%token TOK_INCLUDE
%token TOK_HOOK
%token TOK_AUGMENT


%nonassoc ':'
%left ';'                      
%left ','
%nonassoc TOK_RULE
%nonassoc TOK_FOREIGN                        
%nonassoc TOK_ASSIGN
%right TOK_THEN                        
%nonassoc TOK_HOT TOK_COLD TOK_IDLE TOK_GREEDY
%nonassoc TOK_ELSE
%right TOK_IF TOK_FOR TOK_FOREACH TOK_WHILE TOK_SWITCH TOK_GENERIC TOK_SELECT TOK_FREEZE TOK_WATCH
%nonassoc TOK_KILL TOK_SUSPEND TOK_RESUME TOK_RETURN TOK_YIELD TOK_ERROR TOK_GOTO TOK_ASSERT TOK_EXPECT 
%right TOK_PUT TOK_PUT_ALL TOK_PUT_NEXT 
%left '='
%right '?'
%nonassoc TOK_MATCH_BINDING
%left '|' 
%left '&'
%nonassoc TOK_EQ TOK_NE '<' '>' TOK_LE TOK_GE '~' TOK_NOT_MATCH TOK_IN TOK_ISTYPE
%left TOK_MAX TOK_MIN 
%left '+' '-' TOK_APPEND TOK_CHOP 
%left '*' '/' TOK_DIV TOK_MOD TOK_INTERSPERSE TOK_SPLIT
%right '^'
%left TOK_TYPECAST
%left '[' 
%right '!' TOK_TRACING TOK_PEEK TOK_UMINUS
%left '.'
%nonassoc TOK_ID TOK_REGEXP
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
        |       modulecontents pragma ';'
        |       modulecontents moduleconditional
        |       modulecontents scope declaration
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

scope:          /*empty*/
        |       TOK_EXTERN
        |       TOK_LOCAL
                ;

declaration:    import ';'
        |       nodedecl
                ;

import:         TOK_IMPORT TOK_ID versionconstraint importlist importalias
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

nodedecl:       nodekind TOK_ID '(' nodeargs ')' nodedef
                ;

nodekind:     /*empty*/
        |       TOK_PROTOTYPE
        |       TOK_PARTITION
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

nodeblock:   '{' sequence  '}'
        |    '{' states '}'
                ;

states:         state
        |       states state
        ;

state:          statelabel ':' sequence
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

varref:         TOK_ID
        |       TOK_ID '.' TOK_ID  %prec '.'
        |       TOK_ID '.' TOK_ID '.' TOK_ID %prec '.' 
                ;

hookref:        TOK_ID '.' TOK_ID  %prec '.'
        |       TOK_ID '.' TOK_ID '.' TOK_ID %prec '.' 
                ;

expression: literal
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
        |       expression '[' expression0 ']' 
        |       '+' expression %prec TOK_UMINUS 
        |       '-' expression %prec TOK_UMINUS 
        |       '*' expression %prec TOK_UMINUS 
        |       '^' expression %prec TOK_UMINUS
        |       TOK_TRACING expression
        |       '?' expression  %prec TOK_UMINUS
        |       '&' expression  %prec TOK_UMINUS
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
        |       expression TOK_ELSE expression
        |       expression TOK_MIN expression 
        |       expression TOK_MAX expression
        |       expression TOK_APPEND expression
        |       expression TOK_INTERSPERSE expression
        |       expression TOK_CHOP expression
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
        |       expression TOK_NOT_MATCH pattern
        |       expression TOK_IN expression
        |       expression TOK_ISTYPE typechoice
        |       expression '=' expression
        |       expression TOK_PUT expression
        |       expression TOK_PUT_ALL expression
        |       expression TOK_PUT_NEXT expression
        |       TOK_KILL expression
        |       TOK_SUSPEND expression
        |       TOK_RESUME expression
        |       TOK_YIELD expression
        |       TOK_ERROR expression
        |       TOK_ASSERT expression
        |       TOK_EXPECT expression
        |       TOK_GOTO TOK_ID
        |       TOK_RETURN
        |       TOK_IF '(' expression ')' expression %prec TOK_IF
        |       TOK_WHILE '(' expression ')' expression %prec TOK_WHILE
        |       TOK_FOR '(' bindings ';' expression ';' bindings ')' expression %prec TOK_FOR
        |       TOK_FOREACH '(' foreach_spec ')' expression %prec TOK_FOREACH
        |       TOK_FREEZE '(' expression ')' expression %prec TOK_FREEZE
        |       TOK_SWITCH '(' expression ')' '{' alternatives '}'
        |       TOK_SELECT '{' select_alternatives '}'
        |       TOK_GENERIC '(' expression ')' '{' generic_alternatives '}'
        |       TOK_WATCH '(' expression TOK_RULE expression ')' expression %prec TOK_WATCH
        |       TOK_HOT expression
        |       TOK_COLD expression
        |       TOK_IDLE expression
        |       TOK_GREEDY expression
        |       TOK_FOREIGN  TOK_ID '(' exprlist0 ')' TOK_STRING %prec TOK_FOREIGN
                ;

anonymous_node: '@' noderef block
        |       '@' TOK_FOR  '(' bindings ';' expression0 ';' bindings ')' expression %prec TOK_FOR
        |       '@' TOK_FOREACH '(' foreach_spec ')' expression %prec TOK_FOREACH
                ;

expression0:    /*empty*/
        |       expression
        ;

bindings:       binding 
        |       bindings ';' binding
                ;

binding: TOK_ID TOK_ASSIGN expression
        | pragma
        | TOK_HOOK TOK_ID
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

assoc:          assockey TOK_RULE expression
                ;

assockey:       TOK_ID
        |       literal
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
        |       '&' expression
        ;

pattern:        TOK_ID
        |       TOK_WILDCARD
        |       literal
        |       TOK_REGEXP
        |       '(' expression ')'
        |       TOK_ID TOK_ASSIGN pattern 
        |       TOK_ID '(' patternlist0 ')' %prec '('
        |       '[' patternlist ']'
        |       '[' patternassoclist ']'
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
