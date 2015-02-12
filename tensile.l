%option yylineno

  #include "tensile.tab.h"
  #include "engine.h"

ESCAPE \\([abfnrtv\'\"\\]|u\+?[[:xdigit:]]{4}|U\+?[[:xdigit:]]{8}|[uU]\{[^}]+\})
BINESCAPE \\([abfnrtv\'\"\\]|[0-9]+|[xX][[:xdigit:]]+)
%%
[][{},;:{}~+*/%<>=.^@#|&!()$-] { return *yytext; }
"~~" { return TOK_MATCH_CI; }
"==" { return TOK_EQ; }
"~=" { return TOK_EQ_CI; }
"<~" { return TOK_LESS_CI; }
">~" { return TOK_GREATER_CI; }
"<=" { return TOK_LE; }
"<~=" { return TOK_LE_CI; }
">=" { return TOK_GE; }
">~=" { return TOK_GE_CI; }
"!=" { return TOK_NE; }
"!~=" { return TOK_NE_CI; }
"!~" { return TOK_NOT_MATCH; }
"!~~" { return TOK_NOT_MATCH_CI; }
"<:" { return TOK_MIN; }
"<:~" { return TOK_MIN_CI; }
":>" { return TOK_MAX; }
":>~" { return TOK_MAX_CI; }
".[" { return TOK_VARFIELD; }
import { return TOK_IMPORT; }
pre { return TOK_PRE; }
post { return  TOK_POST; }
default { return TOK_DEFAULT; }

\.[[:alnum:]_]+ { return TOK_FIELD; }
[+-]infinity { return TOK_NUMBER; }
[+-]?([0-9](\.[0-9]+)?|\.[0-9]+)([eE][+-]?[0-9]+)? { return TOK_NUMBER; }
[0-9]{4}-[0-9]{2}-[0-9]{2}T([0-9]{2}:[0-9]{2}(:[0-9]{2})?)? { return TOK_TIMESTAMP; }
B\"([^\"\\]+|{BINESCAPE})*\" { return TOK_BINARY; }
T'[^\']+' { return TOK_TIMESTAMP; }
P([0-9]+Y)?([0-9]+M)?([0-9]+W)?([0-9]+D)?(T([0-9]+H)?([0-9]+M)?([0-9]+S)?)? { return TOK_NUMBER; }
'[^\']+' { return TOK_LITERAL; } 
\"([^\"\\]+|{ESCAPE})*\" { return TOK_LITERAL; }
[[:alpha:]_][[:alnum:]_]* { return TOK_ID; }


[[:space:]]+ {}
"//".*\n {}
"/*"([^\*]+|\*+[^\/\*])+"*/" {}

