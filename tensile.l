%option yylineno
%option bison-bridge
%option bison-locations
%option reentrant
%{

#include <stdlib.h>
#include "vmtypes.h"
#include "ast.h"
#include "tensile.tab.h"

#define YY_DECL int yylex(YYSTYPE* yylval_param,    \
                          YYLTYPE * yylloc_param,   \
                          exec_context *context)

YY_DECL;

#define YY_USER_ACTION {yylloc->first_line = yylineno; \
           yylloc->first_column = 0;                   \
           yylloc->last_column=0;                      \
           yylloc->last_line = yylineno;               \
       }

%}

ESCAPE \\([abfnrtv\'\"\\]|u\+?[[:xdigit:]]{4}|U\+?[[:xdigit:]]{8}|[uU]\{[^}]+\})
%%
[-+*/^!~(){}.?,;<>:&|%=#\\] { return *yytext; }
[][] { return *yytext; }
"[|" { return TOK_LDBRACKET; }
"|]" { return TOK_RDBRACKET; }
"{|" { return TOK_LDBRACE; }
"|}" { return TOK_RDBRACE; }
"!~" { return TOK_NOT_MATCH; }
"==" { return TOK_EQ; }
"<=" { return TOK_LE; }
">=" { return TOK_GE; }
"!=" { return TOK_NE; }
"<:" { return TOK_MIN; }
":>" { return TOK_MAX; }
"->" { return TOK_ARROW; }
"<->" { return TOK_INTERACT; }
"=>" { return TOK_MAP; }
"++" { return TOK_APPEND; }
".." { return TOK_RANGE; }
"||" { return TOK_ALT; }
">>" { return TOK_OUTGOING; }
"<<" { return TOK_INCOMING; }
":=" { return TOK_FORCE_LET; }
else { return TOK_ELSE; }
extern { return TOK_EXTERN; }
false { return TOK_FALSE; }
if { return TOK_IF; }
import { return TOK_IMPORT; }
loop { return TOK_LOOP; }
new { return TOK_NEW; }
override { return TOK_OVERRIDE; }
pragma { return TOK_PRAGMA; }
switch { return TOK_SWITCH; }
true { return TOK_TRUE; }
typeof { return TOK_TYPEOF; }
[+-]?[0-9]+ {
    yylval->ast = ast_create_node(AST_LITERAL,
                                  &vm_root_inttype,
                                  (union vm_value){.ival = apr_strtoi64(yytext, NULL, 10)});
    return TOK_INTEGER;
}
0[xX][[:xdigit:]]+ {
    yylval->ast = ast_create_node(AST_LITERAL,
                                  &vm_root_inttype,
                                  (union vm_value){.ival = apr_strtoi64(yytext + 2, NULL, 16)});
    return TOK_INTEGER;
}
0[bB][01]+ {
    yylval->ast = ast_create_node(AST_LITERAL,
                                  &vm_root_inttype,
                                  (union vm_value){.ival = apr_strtoi64(yytext, NULL, 2)});
    return TOK_INTEGER;
}
[+-]?([0-9]+\.[0-9]*([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+) {
    yylval->ast = ast_create_node(AST_LITERAL,
                                  &vm_root_floattype,
                                  (union vm_value){.dval = strtod(yytext, NULL)});
    return TOK_FLOAT; 
}
[0-9]{4}-[0-9]{2}-[0-9]{2}T([0-9]{2}:[0-9]{2}(:[0-9]{2})?)? {
    time_t ts = parse_iso_timestamp(yytext);
    
    yylval->ast = ast_create_node(AST_LITERAL,
                                  &vm_root_timetype,
                                  (union vm_value){.tval = ts});
    return TOK_TIMESTAMP; 
}
'([^\']|{ESCAPE})' {
    return TOK_CHARACTER; 
}
\"([^\"\\]+|{ESCAPE})*\" {
    return TOK_STRING; }
[[:alpha:]_][[:alnum:]_]* { 
    return TOK_ID; 
}
`([^`\\]+|{ESCAPE})*` {
    return TOK_ID;
}
^#[[:space:]]+[0-9]+[[:space:]]+.*\n {
    yylineno = strtoul(yytext + 1, NULL, 10);
}
"(:"([^:]+|:+[^+)])":)" { return TOK_REGEXP; }
[[:space:]]+ {}
"//".*\n {}
"/*"([^*]+|\*+[^\/*])*"*/" {}
