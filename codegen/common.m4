m4_define(`_STRUCTDEF', `m4_define(`__current__', ``$1'')m4_dnl
m4_define(`__qualifier', `/*@owned@*/')m4_dnl
m4_define(`__wqualifier', `const /*@dependent@*/')m4_dnl
m4_define(`__dtype', `/*@only@*/ /*@out@*/ `$1' *')m4_dnl
m4_define(`__ctype', `/*@only@*/ `$1' *')m4_dnl
m4_define(`__commadef', `m4_define(`__comma', `,')')')m4_dnl
m4_define(`__comma', `__commadef')m4_dnl
m4_define(`_RCSTRUCTDEF', `m4_define(`__qualifier', `/*@refcounted@*/')m4_dnl
m4_define(`__dtype', `/*@killref@*/ `$1'_ptr')m4_dnl
m4_define(`__ctype', `/*@newref@*/ `$1'_ptr')')m4_dnl
m4_define(`_CONARG', `m4_divert(2)__comma `$1'm4_divert(0)')m4_dnl
m4_define(`_OPTCONARG', `_CONARG(`/*@null@*/ $1')')m4_dnl
m4_define(`_STRARG', `_CONARG(`const xmlChar *$1')')m4_dnl
m4_define(`_OPTSTRARG', `_OPTCONARG(`const xmlChar *')')m4_dnl
m4_define(`_QNAMEARG', `_OPTSTRARG(`$1_ns')_STRARG(`$1')')m4_dnl
m4_define(`_OPTQNAMEARG', `_OPTSTRARG(`$1_ns')_OPTSTRARG(`$1')')m4_dnl
m4_define(`_REFARG', `_CONARG(`$1_ptr $2')')m4_dnl
m4_define(`_OPTREFARG', `_OPTCONARG(`$1_ptr $2')')m4_dnl
m4_define(`_WEAKREFARG', `_CONARG(`$1_weakptr $2')')m4_dnl
m4_define(`_OPTWEAKREFARG', `_OPTCONARG(`$1_weakptr $2')')m4_dnl
m4_define(`OUTPUTARGS', `m4_undivert(2)m4_define(`__comma', `__commadef')')m4_dnl
m4_define(`BODY', `m4_divert(1)$1`'m4_divert(0)')m4_dnl