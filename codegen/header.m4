m4_define(`STRUCT', `m4_divert(1)typedef struct `$1' {
m4_define(`__current__', ``$1'')m4_dnl
m4_define(`__qualifier', `/*@owned@*/')m4_dnl
m4_define(`__wqualifier', `const /*@dependent@*/')m4_dnl
m4_define(`__dqualifier', `/*@only@*/ /*@out@*/')m4_dnl
m4_define(`__comma', `m4_define(`__comma', `,')')m4_dnl
m4_divert(2)
extern `$1' *`$1'_new(m4_dnl
m4_divert(0)')m4_dnl
m4_define(`USING', `__using_$1(`$2')')m4_dnl
m4_define(`END', `m4_divert(1)} __current__;
m4_divert(0)m4_dnl
typedef __qualifier struct __current__ *__current__`'_ptr;
typedef __wqualifier struct __current__ *__current__`'_weakptr;
m4_divert(2)m4_dnl
);
extern void __current__`'_destroy(/*@null*@/ __dqualifier *obj) /*@modifies obj*@/;
')m4_dnl
m4_define(`RCSTRUCT', `STRUCT(`$1')m4_divert(1)
m4_define(`__qualifier', `/*@refcounted@*/')m4_dnl
m4_define(`__dqualifier', `/*@killref@*/')m4_dnl
    /*@refs@*/ unsigned refcnt;
m4_divert(0)')m4_dnl
m4_define(`_FIELD', `m4_divert(1)`$1' `$2';
m4_divert(0)')m4_dnl
m4_define(`_CONARG', `m4_divert(2)__comma `$1'm4_divert(0)')m4_dnl
m4_define(`FIELD', `_FIELD(`$1', `$2')_CONARG(`$1 $2')')m4_dnl
m4_define(`STRING', `_FIELD(`/*@owned@*/ xmlChar *', `$1')_CONARG(`const xmlChar *$1')')m4_dnl
m4_define(`OPTSTRING', `_FIELD(`/*@owned@*/ /*@null@*/ xmlChar *', `$1')_CONARG(`/*@null@*/ const xmlChar *$1')')m4_dnl
m4_define(`NCNAME', `_FIELD(`/*@dependent@*/ const xmlChar *', `$1')_CONARG(`const xmlChar *$1')')m4_dnl
m4_define(`OPTNCNAME', `_FIELD(`/*@dependent@*/ /*@null@*/ const xmlChar *', `$1')_CONARG(`/*@null@*/ const xmlChar *$1')')m4_dnl
m4_define(`QNAME', `FIELD(`pipeline_qname', `$1')')m4_dnl
m4_define(`LIST', `_FIELD(`/*@owned@*/ xmlListPtr', `$2')')m4_dnl
m4_define(`WEAKLIST', `LIST(`$1', `$2')')m4_dnl
m4_define(`HASH', `_FIELD(`/*@owned@*/ xmlHashTablePtr', `$2')')m4_dnl
m4_define(`REF', `_FIELD(`$1_ptr', `$2')')m4_dnl
m4_define(`REF', `_FIELD(`$1_ptr', `$2')')m4_dnl
m4_define(`WEAKREF', `FIELD(`$1_weakptr', `$2')')m4_dnl
