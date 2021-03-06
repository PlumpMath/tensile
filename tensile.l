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
[-+*/^!~(){}.?,;<>:&|%=\\] { return *yytext; }
[][] { return *yytext; }
"[|" { return TOK_LDBRACKET; }
"|]" { return TOK_RDBRACKET; }
"!~" { return TOK_NOT_MATCH; }
"~~" { return TOK_MATCH_RESULT; }
"==" { return TOK_EQ; }
"<=" { return TOK_LE; }
">=" { return TOK_GE; }
"!=" { return TOK_NE; }
"->" { return TOK_ARROW; }
"=>" { return TOK_MAP; }
"++" { return TOK_APPEND; }
"--" { return TOK_DISTANCE; }
".." { return TOK_RANGE; }
"::" { return TOK_TYPECHECK; }
assert { return TOK_ASSERT; }
atexit { return TOK_ATEXIT; }
break { return TOK_BREAK; }
case { return TOK_CASE; }
continue { return TOK_CONTINUE; }
dispatch { return TOK_DISPATCH; }
else { return TOK_ELSE; }
enum { return TOK_ENUM; }
exit { return TOK_EXIT; }
extern { return TOK_EXTERN; }
false { return TOK_FALSE; }
for { return TOK_FOR; }
foreach { return TOK_FOREACH; }
gather { return TOK_GATHER; }
generic { return TOK_GENERIC; }
if { return TOK_IF; }
loop { return TOK_LOOP }
import { return TOK_IMPORT; }
nothing { return TOK_NOTHING; }
pragma { return TOK_PRAGMA; }
return { return TOK_RETURN; }
select { return TOK_SELECT; }
sizeof { return TOK_SIZEOF; }
static { return TOK_STATIC; }
struct { return TOK_STRUCT; }
switch { return TOK_SWITCH; }
true { return TOK_TRUE; }
typedef { return TOK_TYPEDEF; }
union { return TOK_UNION; }
while { return TOK_WHILE; }
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
    return TOK_BITSTRING;
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
[uU]?\"([^\"\\]+|{ESCAPE})*\" {
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
"/"[^[:space:]/]+"/" { return TOK_REGEXP; }
[[:space:]]+ {}
"#".*\n {}
"/*"([^*]+|\*+[^\/*])*"*/" {}
