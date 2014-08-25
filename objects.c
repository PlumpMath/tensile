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
#include <stdarg.h>
#include <libxml/xmlmemory.h>
#include "pipeline.h"

output_port_instance *new_output_port_instance(pipeline_step *owner,
                                               port_declaration *decl,
                                               input_port_instance *connected) {
  output_port_instance *p = xmlMalloc(sizeof(*p));

  p->owner     = owner;
  p->decl      = decl;
  p->connected = connected;
  return p;
}

port_source *new_port_source(enum port_source_kind kind, ...) {
  port_source *s = xmlMalloc(sizeof(*s));
  va_list args;

  s->kind = kind;
  va_start(args, kind);
  switch (kind) {
    case PORT_SOURCE_INLINE:
      s->x.doc = va_arg(args, xmlDocPtr);
      break;
    case PORT_SOURCE_DOCUMENT:
      s->x.load.uri = xmlStrdup(va_arg(args, xmlChar *));
      s->x.load.wrapper = s->x.load.wrapper_ns = NULL;
      s->x.load.content_type = NULL;
      break;
    case PORT_SOURCE_DATA:
      s->x.load.uri = xmlStrdup(va_arg(args, xmlChar *));
      s->x.load.wrapper = xmlStrdup(va_arg(args, xmlChar *));
      s->x.load.wrapper_ns = xmlStrdup(va_arg(args, xmlChar *));
      s->x.load.content_type = xmlStrdup(va_arg(args, xmlChar *));
      break;
    case PORT_SOURCE_PIPE:
      s->x.pipe = va_arg(args, output_port_instance *);
      break;
    case PORT_SOURCE_UNRESOLVED:
      s->x.ref.step = xmlStrdup(va_arg(args, xmlChar *));
      s->x.ref.port = xmlStrdup(va_arg(args, xmlChar *));
      break;
    default:
      assert(0);
  }
  va_end(args);
  return s;
}

void destroy_port_source(port_source *s) {
  if (s == NULL)
    return;
  
  switch (s->kind) {
    case PORT_SOURCE_INLINE:
      xmlFreeDoc(s->x.doc);
      break;
    case PORT_SOURCE_DOCUMENT:
    case PORT_SOURCE_DATA:
      xmlFree(s->x.load.uri);
      xmlFree(s->x.load.wrapper);
      xmlFree(s->x.load.wrapper_ns);
      xmlFree(s->x.load.content_type);
      break;
    case PORT_SOURCE_PIPE:
      /* do nothing */
      break;
    case PORT_SOURCE_UNRESOLVED:
      xmlFree(s->x.ref.step);
      xmlFree(s->x.ref.port);
      break;
    default:
      assert(0);
  }
  xmlFree(s);
}

static void port_source_deallocator(xmlLinkPtr l) {
  destroy_port_source(xmlLinkGetData(l));
}

port_connectioon *new_port_connection(const xmlChar *filter) {
  port_connection *p = xmlMalloc(sizeof(*p));

  p->refcnt = 1;
  if (!filter)
    p->filter = NULL;
  else {
    p->filter = xmlXPathCompile(filter);
    if (p->filter == NULL) {
      xmlFree(p);
      return NULL;
    }
  }
  p->sources = xmlListCreate(port_source_deallocator, NULL);
  return p;
}

void destroy_port_connection(port_connection *p) {
  if (p == NULL)
    return;
  assert(p->refcnt);
  if (--p->refcnt > 0)
    return;

  if (p->filter)
    xmlXPathFreeCompExpr(p->filter);
  xmlListDelete(p->sources);
}

enum port_kind {
  PORT_INPUT_DOCUMENT,
  PORT_INPUT_PARAMETERS,
  PORT_OUTPUT
};

typedef struct port_declaration {
  enum port_kind kind;
  const char *name;
  bool sequence;
  bool primary;
  port_connection *default_connection;
} port_declaration;

port_declaration *new_port_declaration(enum port_kind kind,
                                       const xmlChar *name,
                                       bool sequence,
                                       bool primary,
                                       port_connection *default_connection) {
  port_declaration *d = xmlMalloc(sizeof(*d));
  d->kind = kind;
  d->name = xmlStrdup(name);
  d->sequence = sequence;
  d->primary = primary;
  d->default_connection = default_connection;
  return d;
}

void delete_port_declaration(port_declaration *d) {
  xmlFree(d->name);
  destroy_port_connection(d->default_connection);
  xmlFree(d);
}

static void xml_doc_deallocate(xmlLinkPtr l) {
  xmlFreeDoc(xmlLinkGetData(l));
}

input_port_instance *new_input_port_instance(pipeline_step *owner,
                                             port_declaration *decl,
                                             port_connection *connection) {
  input_port_instance *p = xmlMalloc(sizeof(*p));
  p->owner = owner;
  p->decl  = decl;
  p->queue = xmlListCreate(xml_doc_deallocate, NULL);
  p->complete = false;
  p->connection = connection;
  return p;
}

void destroy_input_port_instance(input_port_instance *p) {
  xmlListDelete(p->queue);
  destroy_port_connection(d->connection);
  xmlFree(p);
}

pstep_option_decl *new_pstep_option_decl(const xmlChar *name, 
                                         const xmlChar *defval) {
  pstep_option_decl *pd = xmlMalloc(sizeof(*pd));

  if (defval == NULL) {
    pd->defval = NULL;
  } else {
    pd->deval = xmlXPathCompile(defval);
    if (!pd->defval) {
      xmlFree(pd);
      return NULL;
    }
  }
  pd->name = xmlStrdup(name);
}

void destroy_pstep_option_decl(pstep_option_decl *pd) {
  if (pd == NULL)
    return;
  xmlFree(pd->name);
  if (pd->defval)
    xmlXPathFreeCompExpr(pd->defval);
  xmlFree(pd);
}

pipeline_library *new_pipeline_library(const xmlChar *uri) {
  pipeline_library *l = xmlMalloc(sizeof(*l));
  l->uri = xmlStrdup(uri);
  l->types = xmlHashCreate(16);
  l->pipelines = xmlHashCreate(32);
  l->dyn_library = NULL;
  return l;
}

void destroy_pipeline_library(pipeline_library *pl) {
  if (pl == NULL)
    return;
  
  if (pl->dyn_library) {
    void *hook = NULL;
    if (!xmlModuleSymbol(pl->dyn_library, "on_destroy_library", &hook) &&
        hook != NULL) {
      ((void (*)(pipeline_library *))hook)(pl);
    }
    xmlModuleClose(pl->dyn_library);
  }
  xmlHashFree(pl->types, pipeline_type_deallocate);
  xmlHashFree(pl->pipelines, pipeline_object_deallocate);
  xmlFree(pl->uri);
  xmlFree(pl);
}

pipeline_decl *new_pipeline_decl(const xmlChar *ns,
                                 const xmlChar *name,
                                 const pipeline_atomic_type *type) {
  pipeline_decl *pd = xmlMalloc(sizeof(*pd));
  pd->ns = xmlStrdup(ns);
  pd->name = xmlStrdup(name);
  pd->type = type;
  pd->options = xmlListCreate();
  pd->ports = xmlListCreate();
  pd->body = xmlListCreate();
  pd->imports = xmlListCreate(NULL, NULL);
  return pd;
}

void destroy_pipeline_decl(pipeline_decl *pd) {
  if (pd == NULL)
    return;
  
  xmlFree(pd->ns);
  xmlFree(pd->name);
  xmlListDelete(pd->options);
  xmlListDelete(pd->ports);
  xmlListDelete(pd->body);
  xmlListDelete(pd->imports);
}

pipeline_option *new_pipeline_option(pstep_option_decl *decl,
                                     port_connection *value) {
  pipeline_option *po = xmlMalloc(sizeof(*po));
  po->decl = decl;
  po->value = value;
  return po;
}

void destroy_pipeline_option(pipeline_option *po) {
  if (po == NULL)
    return;
  destroy_port_connection(po->value);
  xmlFree(po);
}

enum pipeline_step_kind {
  PSTEP_END,
  PSTEP_ATOMIC,
  PSTEP_CALL,
  PSTEP_ASSIGN,
  PSTEP_FOREACH,
  PSTEP_VIEWPORT,
  PSTEP_CHOOSE,
  PSTEP_GROUP,
  PSTEP_TRY
};

typedef struct pipeline_assignment {
  const char *ns;
  const char *name;
  port_connection *source;
} pipeline_assignment;

pipeline_assignment *new_pipeline_assignment(const xmlChar *ns,
                                             const xmlChar *name,
                                             port_connection *source) {
  pipeline_assignment *pa = xmlMalloc(sizeof(*pa));
  pa->ns = xmlStrdup(ns);
  pa->name = xmlStrdup(name);
  pa->source = source;
  return pa;
}

void destroy_pipeline_assignment(pipeline_assignment *pa) {
  if (!pa)
    return;
  xmlFree(pa->ns);
  xmlFree(pa->name);
  destroy_port_connection(pa->source);
  xmlFree(pa);
}

pipeline_branch *new_pipeline_branch(port_connection *test) {
  pipeline_branch *pb = xmlMalloc(sizeof(*pb));
  pb->test = test;
  pb->body = xmlListCreate(pipeline_step_deallocate, NULL);
  return pb;
}

typedef struct pipeline_branch {
  port_connection *test;
  xmlListPtr /* pipeline_step */ body;
} pipeline_branch;

typedef struct pipeline_step {
  enum pipeline_step_kind kind;
  xmlListPtr /* input_port_instance */ inputs;
  xmlListPtr /* output_port_instance */ outputs;
  xmlListPtr /* pipeline_option */ options;
  union {
    struct {
      pipeline_decl *decl;
      void *data;
    } atomic;
    pipeline_decl *call;
    pipeline_assignment *assign;
    xmlListPtr /* pipeline_step */ body;
    struct {
      xmlXPathCompExprPtr match;
      xmlListPtr /* pipeline_step */ body;
    } viewport;
    struct {
      xmlListPtr /* pipeline_assignment */ assign;
      xmlListPtr /* pipeline_branch */ branches;
      xmlListPtr /* pipeline_step */ otherwise;
    } choose;
    struct {
      xmlListPtr /* pipeline_assignment */ assign;
      struct pipeline_step *try;
      struct pipeline_step *catch;
    } trycatch;
  } x;
} pipeline_step;

struct pipeline_exec_context;

typedef struct pipeline_exec_context {
  pipeline_decl *decl;
  pipeline_step *current;
  unsigned iter_position;
  unsigned iter_size;
  xmlHashTablePtr bindings;
} pipeline_exec_context;
