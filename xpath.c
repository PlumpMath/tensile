/*
 * Copyright (c) 2014  Artem V. Andreev
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
/**
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */

#include <string.h>
#include <stdlib.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/uri.h>
#include <libxml/xpathInternals.h>
#include "pipeline.h"
#include "version.h"

static void split_qname(xmlXPathParserContextPtr ctx,
                        xmlChar *qname,
                        /*@out@*/ const xmlChar **uri,
                        /*@out@*/ const xmlChar **local) {
  xmlChar *sep = (xmlChar *)xmlStrchr(qname, (xmlChar)':');
  if (sep == NULL) {
    *uri = NULL;
    *local = qname;
  } else {
    *sep = (xmlChar)'\0';
    *uri = xmlXPathNsLookup(ctx->context, qname);
    if (*uri == NULL)
      xmlXPathSetError(ctx, XPATH_UNDEF_PREFIX_ERROR);
    *local = sep + 1;
  }
}

static xmlChar *detect_language(void) {
  static /*@ observer @*/ const char * varnames[] = {
    "LANGUAGE",
    "LANG",
    "LC_MESSAGES",
    "LC_ALL",
    "LC_CTYPE",
  };
  unsigned i;
  const char *lang = NULL;
  const char *sep;

  for (i = 0; lang == NULL && 
         i < (unsigned)(sizeof(varnames) / sizeof(*varnames)); i++) {
    lang = getenv(varnames[i]);
  }
  if (lang == NULL)
    return xmlCharStrdup("");
  sep = strchr(lang, '.');
  if (sep == NULL) 
    return xmlCharStrdup(lang);
  else {
    xmlChar *result = xmlMalloc((size_t)(sep - lang + 1));
    memcpy(result, lang, (size_t)(sep - lang));
    result[sep - lang] = (xmlChar)'\0';
    return result;
  }
}

static void pipeline_xpath_system_property(xmlXPathParserContextPtr ctx, 
                                           int nargs) {
  xmlChar *qname;
  const xmlChar *uri;
  const xmlChar *local;
  
  if (nargs != 1) {
    xmlXPathSetArityError(ctx);
    return;
  }
  qname = xmlXPathPopString(ctx);
  if (xmlXPathCheckError(ctx)) {
    xmlFree(qname);
    return;
  }
  split_qname(ctx, qname, &uri, &local);
  xmlFree(qname);
  if (xmlXPathCheckError(ctx))
    return;
  if (xmlStrcmp(uri, (const xmlChar *)W3C_XPROC_NAMESPACE) != 0) {
    xmlXPathReturnEmptyString(ctx);
    return;
  }
  if (xmlStrcmp(local, (const xmlChar *)"episode") == 0) {
    xmlXPathReturnString(ctx, xmlCharStrdup(xproc_episode));
  } else if (xmlStrcmp(local, (const xmlChar *)"language") == 0) {
    xmlXPathReturnString(ctx, detect_language());
  } else if (xmlStrcmp(local, (const xmlChar *)"product-name") == 0) {
    xmlXPathReturnString(ctx, xmlCharStrdup(XPROC_PRODUCT_NAME));
  } else if (xmlStrcmp(local, (const xmlChar *)"product-version") == 0) {
    xmlXPathReturnString(ctx, xmlCharStrdup(XPROC_PRODUCT_VERSION));
  } else if (xmlStrcmp(local, (const xmlChar *)"vendor") == 0) {
    xmlXPathReturnString(ctx, xmlCharStrdup(XPROC_VENDOR));
  } else if (xmlStrcmp(local, (const xmlChar *)"vendor-uri") == 0) {
    xmlXPathReturnString(ctx, xmlCharStrdup(XPROC_VENDOR_URI));
  } else if (xmlStrcmp(local, (const xmlChar *)"version") == 0) {
    xmlXPathReturnString(ctx, xmlCharStrdup(XPROC_SUPPORTED_VERSION_S));
  } else if (xmlStrcmp(local, (const xmlChar *)"xpath-version") == 0) {
    xmlXPathReturnString(ctx, xmlCharStrdup(XPROC_SUPPORTED_XPATH_VERSION_S));
  } else {
    xmlXPathReturnEmptyString(ctx);
  }
}

static void pipeline_xpath_step_available(xmlXPathParserContextPtr ctx, 
                                           int nargs) {
  xmlChar *qname;
  const xmlChar *uri;
  const xmlChar *local;
  pipeline_exec_context *p_context = ctx->context->extra;
  
  if (nargs != 1) {
    xmlXPathSetArityError(ctx);
    return;
  }
  if (p_context == NULL) {
    xmlXPathReturnFalse(ctx);
    return;
  }
  qname = xmlXPathPopString(ctx);
  if (xmlXPathCheckError(ctx)) {
    xmlFree(qname);
    return;
  }
  split_qname(ctx, qname, &uri, &local);
  xmlFree(qname);
  if (xmlXPathCheckError(ctx))
    return;

  xmlXPathReturnBoolean(ctx, 
                        pipeline_lookup_type(p_context->decl,
                                             uri, local) != NULL);
}

static void pipeline_xpath_value_available(xmlXPathParserContextPtr ctx,
                                           int nargs) {
  xmlChar *qname;
  const xmlChar *uri;
  const xmlChar *local;
  pipeline_exec_context *p_context = ctx->context->extra;
  bool fail_if_unknown = true;
  
  if (nargs > 2 || nargs < 1) {
    xmlXPathSetArityError(ctx);
    return;
  }
  if (p_context == NULL) {
    xmlXPathReturnFalse(ctx);
    return;
  }

  if (nargs == 2) {
    fail_if_unknown = xmlXPathPopBoolean(ctx);
    if (xmlXPathCheckError(ctx))
      return;
  }
  qname = xmlXPathPopString(ctx);
  if (xmlXPathCheckError(ctx)) {
    xmlFree(qname);
    return;
  }
  
  split_qname(ctx, qname, &uri, &local);
  xmlFree(qname);
  if (xmlXPathCheckError(ctx))
    return;
  
  if (xmlHashLookup2(p_context->bindings,
                     (xmlChar *)local, (xmlChar *)uri) != NULL)
    xmlXPathReturnTrue(ctx);
  else if (!fail_if_unknown) 
    xmlXPathReturnFalse(ctx);
  else {
    xmlXPathSetError(ctx, XPATH_UNDEF_VARIABLE_ERROR);
  }
}

static void pipeline_xpath_iter_position(xmlXPathParserContextPtr ctx,
                                         int nargs) {
  pipeline_exec_context *p_context = ctx->context->extra;

  if (nargs > 0) {
    xmlXPathSetArityError(ctx);
    return;
  }

  xmlXPathReturnNumber(ctx, p_context != NULL ? 
                       (double)p_context->iter_position : 1.0);
}

static void pipeline_xpath_iter_size(xmlXPathParserContextPtr ctx,
                                     int nargs) {
  pipeline_exec_context *p_context = ctx->context->extra;

  if (nargs > 0) {
    xmlXPathSetArityError(ctx);
    return;
  }

  xmlXPathReturnNumber(ctx, p_context != NULL ? 
                       (double)p_context->iter_size : 1.0);
}

static void pipeline_xpath_base_uri(xmlXPathParserContextPtr ctx,
                                    int nargs) {
  xmlNodePtr node = xmlXPathGetContextNode(ctx);
  xmlChar *base;
  
  if (nargs > 1) {
    xmlXPathSetArityError(ctx);
    return;  
  }
  if (nargs == 1) {
    xmlNodeSetPtr nodes = xmlXPathPopNodeSet(ctx);
    
    if (xmlXPathCheckError(ctx)) {
      xmlXPathFreeNodeSet(nodes);
      return;
    }
    node = xmlXPathNodeSetItem(nodes, 0);
    xmlXPathFreeNodeSet(nodes);
    if (node == NULL) {
      xmlXPathReturnEmptyString(ctx);
      return;
    }
  }
  base = xmlNodeGetBase(xmlXPathGetDocument(ctx), node);
  if (base == NULL)
    xmlXPathReturnEmptyString(ctx);
  else
    xmlXPathReturnString(ctx, base);
}

static void pipeline_xpath_resolve_uri(xmlXPathParserContextPtr ctx,
                                    int nargs) {
  xmlChar *base = NULL;
  xmlChar *uri;
  xmlChar *result;
  
  if (nargs > 2) {
    xmlXPathSetArityError(ctx);
    return;  
  }
  if (nargs == 2) {
    base = xmlXPathPopString(ctx);
    if (xmlXPathCheckError(ctx)) {
      xmlFree(base);
      return;
    }
  } else {
    base = xmlNodeGetBase(xmlXPathGetDocument(ctx), 
                          xmlXPathGetContextNode(ctx));
  }
  // if no base, just leave URI string as is
  if (base == NULL)
    return;
  uri = xmlXPathPopString(ctx);
  if (xmlXPathCheckError(ctx)) {
    xmlFree(base);
    xmlFree(uri);
    return;
  }
  result = xmlBuildURI(uri, base);
  xmlFree(uri);
  xmlFree(base);
  if (result == NULL) {
    xmlXPathSetError(ctx, XPATH_INVALID_OPERAND);
    return;
  }
  xmlXPathReturnString(ctx, result);
}

static void pipeline_xpath_version_available(xmlXPathParserContextPtr ctx,
                                             int nargs) {
  double val;
  if (nargs != 1) {
    xmlXPathSetArityError(ctx);
    return;
  }
  val = xmlXPathPopNumber(ctx);
  if (xmlXPathCheckError(ctx))
    return;
  
  xmlXPathReturnBoolean(ctx, val <= XPROC_SUPPORTED_VERSION);
}

static void pipeline_xpath_xpath_version_available(xmlXPathParserContextPtr ctx,
                                                   int nargs) {
  double val;
  if (nargs != 1) {
    xmlXPathSetArityError(ctx);
    return;
  }
  val = xmlXPathPopNumber(ctx);
  if (xmlXPathCheckError(ctx))
    return;
  
  xmlXPathReturnBoolean(ctx, val <= XPROC_SUPPORTED_XPATH_VERSION);
}

static xmlXPathObjectPtr pipeline_xpath_lookup_binding(void *ctxt,
                                                       const xmlChar *name,
                                                       const xmlChar *ns_uri) {
  xmlXPathObjectPtr obj = 
    xmlHashLookup2(((pipeline_exec_context *)ctxt)->bindings, name, ns_uri);
  return obj != NULL ? xmlXPathObjectCopy(obj) : NULL;
}


void pipeline_xpath_update_doc(xmlXPathContextPtr ctx, xmlDocPtr doc) {
  static /*@ owned @*/ xmlDocPtr xproc_empty_doc = NULL;
  
  if (doc == NULL) {
    if (xproc_empty_doc == NULL)
      xproc_empty_doc = xmlNewDoc((const xmlChar *)"1.0");
    doc = xproc_empty_doc;
  }
  ctx->doc = doc;
  ctx->node = xmlDocGetRootElement(ctx->doc);
}

void pipeline_xpath_update_context(xmlXPathContextPtr ctx, /*@ dependent @*/ pipeline_exec_context *pctxt) {
  ctx->extra = pctxt;
  xmlXPathRegisterVariableLookup(ctx, 
                                 pipeline_xpath_lookup_binding, pctxt);
}

xmlXPathContext *pipeline_xpath_create_static_context(xmlDocPtr xproc_doc, 
                                                      xmlNodePtr xproc_node) {
  xmlXPathContextPtr newctx;
  xmlNsPtr *nslist;
  static const struct {
    /*@ observer @*/ const char *name;
    xmlXPathFunction func;
  } xpath_functions[] = {
    {"system-property", pipeline_xpath_system_property},
    {"step-available", pipeline_xpath_step_available},
    {"value-available", pipeline_xpath_value_available},
    {"iteration-position", pipeline_xpath_iter_position},
    {"iteration-size", pipeline_xpath_iter_size},
    {"base-uri", pipeline_xpath_base_uri},
    {"resolve-uri", pipeline_xpath_resolve_uri},
    {"version-available", pipeline_xpath_version_available},
    {"xpath-version-available", pipeline_xpath_xpath_version_available},
  };
  unsigned i = 0;

  newctx = xmlXPathNewContext(NULL);
  if (newctx == NULL)
    return NULL;
  pipeline_xpath_update_doc(newctx, NULL);
  newctx->contextSize = newctx->proximityPosition = 1;
  newctx->extra = NULL;
  nslist = xmlGetNsList(xproc_doc, xproc_node);
  if (nslist != NULL) {
    xmlNsPtr *ns;
    for (ns = nslist; *ns != NULL; ns++) {
      (void)xmlXPathRegisterNs(newctx, (*ns)->prefix, (*ns)->href);
    }
    xmlFree(nslist);
  }
  for (i = 0; i < (unsigned)(sizeof(xpath_functions) / sizeof(*xpath_functions)); i++) {
    (void)xmlXPathRegisterFuncNS(newctx, 
                                 (const xmlChar *)xpath_functions[i].name, 
                                 (const xmlChar *)W3C_XPROC_NAMESPACE,
                                 xpath_functions[i].func);
  }
  pipeline_run_hooks("on_xpath_create_static_context", newctx);
  return newctx;
}

