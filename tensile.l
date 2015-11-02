%option yylineno
%option bison-bridge
%option bison-locations
%option reentrant
%{

#include <stdlib.h>
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
_ { return TOK_WILDCARD; }
[-+*/^!~(){}.@?,;<>:&|#] { return *yytext; }
[][] { return *yytext; }
"!~" { return TOK_NOT_MATCH; }
"~~~" { return TOK_MATCH_BINDING_ALL; }
"~~" { return TOK_MATCH_BINDING; }
"==" { return TOK_EQ; }
"<=" { return TOK_LE; }
">=" { return TOK_GE; }
"!=" { return TOK_NE; }
"<:" { return TOK_MIN; }
":>" { return TOK_MAX; }
"=>" { return TOK_RULE; }
"<-" { return TOK_PUT_BACK; }
"->" { return TOK_PUT; }
"->>" { return TOK_PUT_ALL; }
"->!" { return TOK_PUT_NEXT; }
"*?" { return TOK_PEEK; }
"++" { return TOK_APPEND; }
"--" { return TOK_CHOP; }
"-/-" { return TOK_CHOP_HEAD; }
"**" { return TOK_INTERSPERSE; }
"//" { return TOK_SPLIT; }
"<>" { return TOK_QUEUE; }
"/"([^/\\[:space:]]|\\.)+"/" { return TOK_REGEXP; }
as { return TOK_TYPECAST; }
assert { return TOK_ASSERT; }
augment { return TOK_AUGMENT; }
cold { return TOK_COLD; }
div { return TOK_DIV; }
else { return TOK_ELSE; }
end { return TOK_END; }
error { return TOK_ERROR; }
extern { return TOK_EXTERN; }
false { return TOK_FALSE; }
for { return TOK_FOR; }
foreach { return TOK_FOREACH; }
foreign { return TOK_FOREIGN; }
freeze { return TOK_FREEZE; }
goto { return TOK_GOTO; }
greedy { return TOK_GREEDY; }
hook { return TOK_HOOK; }
hot { return TOK_HOT; }
idle { return TOK_IDLE; }
if { return TOK_IF; }
in { return TOK_IN; }
is { return TOK_ISTYPE; }
import { return TOK_IMPORT; }
include { return TOK_INCLUDE; }
kill { return TOK_KILL; }
local { return TOK_LOCAL; }
me { return TOK_ME; }
mod { return TOK_MOD; }
module { return TOK_MODULE; }
need { return TOK_NEED; }
null { yylval->value = NULL_VALUE;  return TOK_NULL; }
partition { return TOK_PARTITION; }
poll { return TOK_POLL; }
pragma { return TOK_PRAGMA; }
prototype { return TOK_PROTOTYPE; }
resume { return TOK_RESUME; }
return { return TOK_RETURN; }
suspend { return TOK_SUSPEND; }
switch { return TOK_SWITCH; }
then { return TOK_THEN; }
tracing { return TOK_TRACING; }
true { return TOK_TRUE; }
typecase { return TOK_TYPECASE; }
watch { return TOK_WATCH; }
while { return TOK_WHILE; }
yield { return TOK_YIELD; }
[+-]?[0-9]+ {
    return TOK_INTEGER;
}
0[xX][[:xdigit:]]+ {
    return TOK_INTEGER;
}
0[bB][01]+ {
    return TOK_INTEGER;
}
[+-]?(([0-9]+\.[0-9]*|\.[0-9]+)([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+) { 
    return TOK_FLOAT; 
}
[0-9]{4}-[0-9]{2}-[0-9]{2}T([0-9]{2}:[0-9]{2}(:[0-9]{2})?)? { 
    yylval->value = parse_iso_timestamp(context, yytext);
    return TOK_TIMESTAMP; 
}
'([^\']|{ESCAPE})' {
    return TOK_CHARACTER; 
} 

\"([^\"\\]+|{ESCAPE})*\" {
    return TOK_STRING; }
\{\{([^}]+|\}[^}])*\}\} {
    return TOK_STRING;
 }
[[:alpha:]_][[:alnum:]_]* { 
    return TOK_ID; 
}

^#[[:space:]]+[0-9]+[[:space:]]+.*\n {
    yylineno = strtoul(yytext + 1, NULL, 10);
}
^#.*\n {}
[[:space:]]+ {}
"/*"([^*]+|\*+[^\/*])*"*/" {}
