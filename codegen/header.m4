m4_define(`STRUCT', `typedef struct `$1' {m4_define(`__current__', ``$1'')m4_dnl
m4_define(`__qualifier', `/*@owned@*/')m4_dnl
m4_define(`__wqualifier', `const /*@dependent@*/')m4_dnl
m4_define(`__dqualifier', `/*@only@*/ /*@out@*/')m4_dnl
')m4_dnl
m4_define(`END', `} __current__;
m4_divert(1)m4_dnl
typedef __qualifier struct __current__ *__current__`'_ptr;
typedef __wqualifier struct __current__ *__current__`'_weakptr;
m4_divert(2)m4_dnl
')m4_dnl
m4_define(`RCSTRUCT', `STRUCT(`$1')
m4_define(`__qualifier', `/*@refcounted@*/')m4_dnl
/*@refs@*/ unsigned refcnt;
')m4_dnl
m4_define(`FIELD', ``$1' `$2';
')m4_dnl
m4_define(`STRING', `FIELD(`/*@owned@*/ /*@null@*/ xmlChar *', `$1')')m4_dnl
m4_define(`NCNAME', `FIELD(`/*@dependent@*/ /*@null@*/ const xmlChar *', `$1')')m4_dnl
m4_define(`QNAME', `FIELD(`pipeline_qname', `$1')')m4_dnl
m4_define(`LIST', `FIELD(`/*@owned@*/ xmlListPtr', `$2')')m4_dnl
m4_define(`WEAKLIST', `LIST(`$1', `$2')')m4_dnl
m4_define(`HASH', `FIELD(`/*@owned@*/ xmlHashTablePtr', `$2')')m4_dnl
m4_define(`REF', `FIELD(`$1_ptr', `$2')')m4_dnl
m4_define(`WEAKREF', `FIELD(`$1_weakptr', `$2')')m4_dnl
