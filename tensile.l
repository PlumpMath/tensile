%option yylineno
%option bison-bridge
%option bison-locations
%{

#include <stdlib.h>
#include "engine.h"
#include "tensile.tab.h"
#include "ops.h"

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
[][{},;:{}~+*/%<>=^@#|&?`!()$-] { return *yytext; }
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
"+=" { 
    yylval->oper = expr_op_plus_assign; 
    return TOK_COMP_ASSIGN; 
}
"&=" { 
    yylval->oper = expr_op_join_assign;
    return TOK_COMP_ASSIGN; 
}
"?=" { 
    yylval->oper = expr_op_default_assign;
    return TOK_COMP_ASSIGN; 
}
"===" { return TOK_IDENTITY; }
"!===" { return TOK_NOT_IDENTITY; }
"->" { return TOK_ARROW; }
".." { return TOK_PARENT; }
^[[:space:]]*require[[:space:]]+[[:alnum:]_]+(\.[[:alnum:]_]+)*([[:space:]]+[+-]?([0-9](\.[0-9]+)?|\.[0-9]+))?[[:space:]]*; {
    char *tokstate = NULL;
    char *modulename = NULL;
    char *modulever = NULL;
    
    apr_strtok(yytext, " \t\n\f\r\v", &tokstate); // skip 'require'
    modulename = apr_strtok(NULL, " \t\n\f\r\v", &tokstate);
    modulever = apr_strtok(NULL, " \t\n\f\r\v;", &tokstate);
    
    if (require_module(context, (const uint8_t *)modulename,
                       modulever ? strtod(modulever, NULL) : 0.0))
    {
        lexer_buffer *last;

        last = &APR_ARRAY_IDX(context->conf->lexer_buffers, 
                              context->conf->lexer_buffers->nelts - 2,
                              lexer_buffer);
        last->lineno = yylineno;
        last = &APR_ARRAY_IDX(context->conf->lexer_buffers, 
                              context->conf->lexer_buffers->nelts - 1,
                              lexer_buffer);
        last->buffer_state = yy_create_buffer(last->stream, YY_BUF_SIZE);
        yy_switch_to_buffer(last->buffer_state);
    }    
}
const { return TOK_CONST; }
extern { return TOK_EXTERN; }
local { return TOK_LOCAL; }
this { return TOK_THIS; }
pragma { return TOK_PRAGMA; }
pre { return TOK_PRE; }
post { return  TOK_POST; }
default { return TOK_DEFAULT; }
typeof { return TOK_TYPEOF; }
as { return TOK_AS; }
null { 
    yylval->value = NULL_VALUE;
    return TOK_NULL; 
}

\.[[:alnum:]_]+ { 
    yylval->str = (uint8_t *)apr_pstrdup(context->static_pool, yytext + 1);
    return TOK_FIELD; 
}
[+-]?infinity { 
    yylval->value = MAKE_NUM_VALUE(*yytext == '-' ? -INFINITY : INFINITY);
    return TOK_NUMBER; 
}
[+-]?([0-9](\.[0-9]+)?|\.[0-9]+)([eE][+-]?[0-9]+)? { 
    yylval->value = MAKE_NUM_VALUE(strtod(yytext, NULL));
    return TOK_NUMBER; 
}
[0-9]{4}-[0-9]{2}-[0-9]{2}T([0-9]{2}:[0-9]{2}(:[0-9]{2})?)? { 
    yylval->value = parse_iso_timestamp(context, yytext);
    return TOK_TIMESTAMP; 
}
P([0-9]+Y)?([0-9]+M)?([0-9]+W)?([0-9]+D)?(T([0-9]+H)?([0-9]+M)?([0-9]+S)?)? { 
    yylval->value = MAKE_NUM_VALUE(parse_duration(context, yytext + 1));
    return TOK_NUMBER; }
'[^\']+' { 
    yytext[yyleng - 1] = '\0';
    yylval->value = MAKE_VALUE(STRING, context->static_pool,
                               (uint8_t *)apr_pstrdup(context->static_pool, yytext + 1));
    return TOK_STRING; 
} 
"<<<"([^>]+|>[^>])*">>>" {
    yytext[yyleng - 3] = '\0';
    yylval->value = MAKE_VALUE(STRING, context->static_pool,
                               (uint8_t *)apr_pstrdup(context->static_pool, yytext + 3));
    return TOK_STRING; 
}
\"([^\"\\]+|{ESCAPE})*\" { 
    yytext[yyleng - 1] = '\0';
    yylval->value = unquote_string(context, yytext + 1);
    return TOK_STRING; }
[[:alpha:]_][[:alnum:]_]* { 
    yylval->str = (uint8_t *)apr_pstrdup(context->static_pool, yytext);
    return TOK_ID; 
}

^#!.*\n {}
^#[[:space:]]+[0-9]+[[:space:]]+.*\n {
    yylineno = strtoul(yytext + 1, NULL, 10);
}
[[:space:]]+ {}
"//".*\n {}
"/*"([^*]+|\*+[^\/*])*"*/" {}
<<EOF>> {
    lexer_buffer *last;
    
    yy_delete_buffer(YY_CURRENT_BUFFER);
    end_module_parsing(context);

    if (apr_is_empty_array(context->conf->lexer_buffers))
        yyterminate();
    last = &APR_ARRAY_IDX(context->conf->lexer_buffers, 
                          context->conf->lexer_buffers->nelts - 1,
                          lexer_buffer);
    yy_switch_to_buffer(last->buffer_state);
    yylineno = last->lineno;
}
