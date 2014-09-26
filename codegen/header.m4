m4_define(`STRUCT', `_STRUCTDEF(`$1')m4_dnl
m4_divert(1)typedef struct `$1' {
m4_divert(2)
m4_divert(0)')m4_dnl
m4_define(`END', `m4_divert(1)} __current__;
m4_divert(0)m4_dnl
typedef __qualifier struct __current__ *__current__`'_ptr;
typedef __wqualifier struct __current__ *__current__`'_weakptr;
m4_divert(2)m4_dnl
extern `$1' *`$1'_new(m4_dnl
);     
extern void __current__`'_destroy(/*@null*@/ __dqualifier __current__ *obj) /*@modifies obj*@/;
')m4_dnl
m4_define(`RCSTRUCT', `STRUCT(`$1')_RCSTRUCTDEF(`$1')m4_divert(1)
    /*@refs@*/ unsigned refcnt;
m4_divert(3)m4_dnl
static inline
/*@newref@*/
`$1_ptr'
`$1_nonull_use'(`$1_ptr' obj)
  /*@modifies obj @*/
{
  obj->refcnt++;
  /*@-refcounttrans @*/
  return obj;
  /*@=refcounttrans @*/
}

static inline
/*@unused@*/ /*@null@*/ /*@newref@*/
`$1_ptr'
`$1_use'(/*@null@*/ `$1_ptr' obj) {
  if (obj == NULL)
    return NULL;
  return `$1_nonull_use'(obj);
}
m4_divert(0)')m4_dnl
m4_define(`_FIELD', `m4_divert(1)`$1' `$2';
m4_divert(0)')m4_dnl
m4_define(`FIELD', `_FIELD(`$1', `$2')_CONARG(`$1 $2')')m4_dnl
m4_define(`STRING', `_FIELD(`/*@owned@*/ xmlChar *', `$1')_STRARG(`$1')')m4_dnl
m4_define(`OPTSTRING', `_FIELD(`/*@owned@*/ /*@null@*/ xmlChar *', `$1')m4_dnl
_OPTSTRARG(`$1')')m4_dnl
m4_define(`NCNAME', `_FIELD(`/*@dependent@*/ const xmlChar *', `$1')m4_dnl
_STRARG(`$1')')m4_dnl
m4_define(`OPTNCNAME', `_FIELD(`/*@dependent@*/ /*@null@*/ const xmlChar *', `$1')m4_dnl
_OPTSTRARG(`$1')')m4_dnl
m4_define(`QNAME', `_FIELD(`/*@null@*/ /*@dependent@*/ const xmlChar *', `$1_ns')m4_dnl
_FIELD(`/*@dependent@*/ const xmlChar *', `$1')m4_dnl
_QNAMEARG(`$1')')m4_dnl
m4_define(`OPTQNAME', `_FIELD(`/*@null@*/ /*@dependent@*/ const xmlChar *', `$1_ns')m4_dnl
_FIELD(`/*@null@*/ /*@dependent@*/ const xmlChar *', `$1')m4_dnl
_OPTQNAMEARG(`$1')')m4_dnl
m4_define(`LIST', `_FIELD(`/*@owned@*/ xmlListPtr', `$2')')m4_dnl
m4_define(`WEAKLIST', `LIST(`$1', `$2')')m4_dnl
m4_define(`HASH', `_FIELD(`/*@owned@*/ xmlHashTablePtr', `$2')')m4_dnl
m4_define(`WEAKHASH', `HASH(`$1', `$2')')m4_dnl
m4_define(`REF', `_FIELD(`$1_ptr', `$2')m4_dnl
_REFARG(`$1', `$2')')m4_dnl
m4_define(`OPTREF', `_FIELD(`/*@null@*/ $1_ptr', `$2')m4_dnl
_OPTREFARG(`$1', `$2')')m4_dnl
m4_define(`WEAKREF', `_FIELD(`$1_weakptr', `$2')m4_dnl
_WEAKREFARG(`$1', `$2')')m4_dnl
m4_define(`OPTWEAKREF', `_FIELD(`/*@null@*/ $1_weakptr', `$2')m4_dnl
_OPTWEAKREFARG(`$1', `$2')')m4_dnl
