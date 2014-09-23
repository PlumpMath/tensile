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

#include <assert.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "pipeline.h"

static void process_use_when(xmlDocPtr doc) 
  /*@modifies: doc @*/ {
  xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
  xmlXPathObjectPtr result = 
    xmlXPathEvalExpression((const xmlChar *)
                           "//*[namespace-uri() = 'http://www.w3.org/ns/xproc']/@use-when | "
                           "//*/@*[namespace-uri() = 'http://www.w3.org/ns/xproc' and local-name() = 'use-when']",
                           ctx);
  int size;
  int i;

  pipeline_add_cleanup((pipeline_cleanup_func)xmlXPathFreeContext, ctx, &ctx);
  if (result == NULL) {
    pipeline_report_xml_error(xmlGetLastError());
  }

  pipeline_add_cleanup((pipeline_cleanup_func)xmlXPathFreeObject, result, &ctx);
  assert(result->type == XPATH_NODESET);
  size = xmlXPathNodeSetGetLength(result->nodesetval);
  for (i = size - 1; i >= 0; i--) {
    xmlNodePtr node = xmlXPathNodeSetItem(result->nodesetval, i);
    xmlXPathContextPtr nodectx;
    xmlXPathObjectPtr cond;
    xmlChar *expr;
    
    assert(node->type == XML_ATTRIBUTE_NODE);
    nodectx = pipeline_xpath_create_static_context(doc, node->parent);
    expr = xmlNodeGetContent(node);
    cond = xmlXPathEvalExpression(expr, nodectx);
    xmlFree(expr);
    if (cond == NULL) {
      xmlXPathFreeContext(nodectx);
      pipeline_report_xml_error(xmlGetLastError());
    }
    if (!xmlXPathCastToBoolean(cond)) {
      xmlUnlinkNode(node->parent);
      xmlFreeNode(node->parent);
    } else {
      xmlUnlinkNode(node);
      xmlFreeNode(node);
    }
    result->nodesetval->nodeTab[i] = NULL;
    xmlXPathFreeObject(cond);
    xmlXPathFreeContext(nodectx);
  }
  pipeline_cleanup_frame(&ctx);
}

static xmlDocPtr read_xproc_document(const char *url) {
  xmlDocPtr doc = xmlReadFile(url, NULL,
                              XML_PARSE_NOENT |
                              XML_PARSE_NOWARNING |
                              XML_PARSE_NOCDATA);
  if (doc == NULL)
    pipeline_report_xml_error(xmlGetLastError());

  pipeline_run_protected((pipeline_protected_func)process_use_when,
                         (pipeline_cleanup_func)xmlFreeDoc,
                         doc);

  return doc;
}

static pipeline_decl *process_pipeline(xmlNodePtr node, bool compat) {
  xmlNodePtr attr;
  for (attr = node->children; attr != NULL; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE && attr->ns == NULL) {
    }
  }
}

