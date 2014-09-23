/*
 * Summary: lists interfaces
 * Description: this module implement the list support used in 
 * various place in the library.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Gary Pennington <Gary.Pennington@uk.sun.com>
 */

#ifndef __XML_LINK_INCLUDE__
#define __XML_LINK_INCLUDE__

#include <libxml/xmlversion.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _xmlLink xmlLink;
typedef xmlLink *xmlLinkPtr;

typedef struct _xmlList xmlList;
typedef xmlList *xmlListPtr;

/**
 * xmlListDeallocator:
 * @lk:  the data to deallocate
 *
 * Callback function used to free data from a list.
 */
typedef void (*xmlListDeallocator) (xmlLinkPtr lk);
/**
 * xmlListDataCompare:
 * @data0: the first data
 * @data1: the second data
 *
 * Callback function used to compare 2 data.
 *
 * Returns 0 is equality, -1 or 1 otherwise depending on the ordering.
 */
typedef int  (*xmlListDataCompare) (/*@null@*/ const void *data0, /*@null@*/ const void *data1);
/**
 * xmlListWalker:
 * @data: the data found in the list
 * @user: extra user provided data to the walker
 *
 * Callback function used when walking a list with xmlListWalk().
 *
 * Returns 0 to stop walking the list, 1 otherwise.
 */
typedef int (*xmlListWalker) (const void *data, const void *user);

/* Creation/Deletion */
XMLPUBFUN xmlListPtr XMLCALL
xmlListCreate		(/*@ null @*/ xmlListDeallocator deallocator,
                     /*@ null @*/ xmlListDataCompare compare);
XMLPUBFUN void XMLCALL		
xmlListDelete		(/*@ only @*/ /*@ out @*/ xmlListPtr l)
  /*@modifies l@*/;

/* Basic Operators */
XMLPUBFUN /*@dependent@*/ /*@null@*/ void * XMLCALL		
		xmlListSearch		(xmlListPtr l,
                             /*@null@*/ void *data);
XMLPUBFUN /*@dependent@*/ /*@null@*/ void * XMLCALL		
		xmlListReverseSearch	(xmlListPtr l,
                                 /*@null@*/ void *data);
XMLPUBFUN int XMLCALL		
		xmlListInsert		(xmlListPtr l,
					 void *data) ;
XMLPUBFUN int XMLCALL		
		xmlListAppend		(xmlListPtr l,
					 void *data) ;
XMLPUBFUN int /*@alt: void@*/ XMLCALL		
		xmlListRemoveFirst	(xmlListPtr l,
                             /*@null@*/ void *data);
XMLPUBFUN int /*@alt: void@*/ XMLCALL		
		xmlListRemoveLast	(xmlListPtr l,
                             /*@null@*/ void *data);
XMLPUBFUN int /*@alt: void@*/ XMLCALL		
		xmlListRemoveAll	(xmlListPtr l,
                             /*@null@*/ void *data);
XMLPUBFUN void XMLCALL		
		xmlListClear		(xmlListPtr l);
XMLPUBFUN int XMLCALL		
		xmlListEmpty		(xmlListPtr l);
XMLPUBFUN /*@dependent@*/ /*@null@*/ xmlLinkPtr XMLCALL	
		xmlListFront		(xmlListPtr l);
XMLPUBFUN /*@dependent@*/ /*@null@*/ xmlLinkPtr XMLCALL	
		xmlListEnd		(xmlListPtr l);
XMLPUBFUN int XMLCALL		
		xmlListSize		(xmlListPtr l);

XMLPUBFUN void XMLCALL		
		xmlListPopFront		(xmlListPtr l)
  /*@modifies l @*/;
XMLPUBFUN void XMLCALL		
		xmlListPopBack		(xmlListPtr l)
  /*@modifies l @*/;
XMLPUBFUN int /*@alt void @*/ XMLCALL		
		xmlListPushFront	(xmlListPtr l,
                             /*@ keep @*/ /*@ null @*/
                             void *data)
  /*@modifies l @*/;
XMLPUBFUN int /*@alt void @*/ XMLCALL		
		xmlListPushBack		(xmlListPtr l,
                             /*@ keep @*/ /*@ null @*/
                             void *data)
  /*@modifies l @*/;

/* Advanced Operators */
XMLPUBFUN void XMLCALL		
		xmlListReverse		(xmlListPtr l);
XMLPUBFUN void XMLCALL		
		xmlListSort		(xmlListPtr l);
XMLPUBFUN void XMLCALL		
		xmlListWalk		(xmlListPtr l,
					 xmlListWalker walker,
					 const void *user);
XMLPUBFUN void XMLCALL		
		xmlListReverseWalk	(xmlListPtr l,
					 xmlListWalker walker,
					 const void *user);
XMLPUBFUN void XMLCALL		
		xmlListMerge		(xmlListPtr l1,
					 xmlListPtr l2);
XMLPUBFUN xmlListPtr XMLCALL	
		xmlListDup		(const xmlListPtr old);
XMLPUBFUN int XMLCALL		
		xmlListCopy		(xmlListPtr cur,
					 const xmlListPtr old);
/* Link operators */
XMLPUBFUN /*@ exposed @*/ void * XMLCALL          
xmlLinkGetData          (/*@ returned @*/ xmlLinkPtr lk);

/* xmlListUnique() */
/* xmlListSwap */

#ifdef __cplusplus
}
#endif

#endif /* __XML_LINK_INCLUDE__ */
