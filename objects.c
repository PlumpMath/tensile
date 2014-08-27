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

output_port_instance *new_output_port_instance(/*@ dependent @*/ pipeline_step *owner,
                                               /*@ dependent @*/ port_declaration *decl,
                                               /*@ null @*/ /*@ dependent @*/ input_port_instance *connected) {
  output_port_instance *p = xmlMalloc(sizeof(*p));

  p->owner     = owner;
  p->decl      = decl;
  p->connected = connected;
  return p;
}

void destroy_output_port_instance(/*@ only @*/ output_port_instance *p) {
  xmlFree(p);
}

static void output_port_instance_deallocator(/*@ only @*/ xmlLinkPtr l) {
  destroy_output_port_instance(xmlLinkGetData(l));
}

static int output_port_instance_search_name(const void *data0,
                                            const void *data1) {
  return xmlStrcmp(((const output_port_instance *)data0)->decl->name,
                   ((const output_port_instance *)data1)->decl->name);
}

/*@ only @*/ /*@ null @*/
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
      abort();
  }
  va_end(args);
  return s;
}

void destroy_port_source(/*@null@*/ /*@only@*/port_source *s) 
/*@modifies s @*/ {
  if (s == NULL)
    return;
  
  switch (s->kind) {
    case PORT_SOURCE_INLINE:
      xmlFreeDoc(s->x.doc);
      break;
    case PORT_SOURCE_DOCUMENT:
    case PORT_SOURCE_DATA:
      xmlFree((xmlChar *)s->x.load.uri);
      xmlFree((xmlChar *)s->x.load.wrapper);
      xmlFree((xmlChar *)s->x.load.wrapper_ns);
      xmlFree((xmlChar *)s->x.load.content_type);
      break;
    case PORT_SOURCE_PIPE:
      /* do nothing */
      break;
    case PORT_SOURCE_UNRESOLVED:
      xmlFree((xmlChar *)s->x.ref.step);
      xmlFree((xmlChar *)s->x.ref.port);
      break;
    default:
      assert(0);
  }
  xmlFree(s);
}

static void port_source_deallocator(/*@ only @*/ xmlLinkPtr l) {
  destroy_port_source(xmlLinkGetData(l));
}

/*@ null @*/ /*@ newref @*/
port_connection *new_port_connection(const xmlChar *filter) {
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

void destroy_port_connection(/*@ null @*/ port_connection_ptr p) {
  if (p == NULL)
    return;
  assert(p->refcnt != 0);
  if (--p->refcnt > 0)
    return;

  if (p->filter)
    xmlXPathFreeCompExpr(p->filter);
  xmlListDelete(p->sources);
  xmlFree(p);
}

port_declaration *new_port_declaration(enum port_kind kind,
                                       const xmlChar *name,
                                       bool sequence,
                                       bool primary,
                                       /*@ only @*/ port_connection *default_connection) {
  port_declaration *d = xmlMalloc(sizeof(*d));
  d->kind = kind;
  d->name = xmlStrdup(name);
  d->sequence = sequence;
  d->primary = primary;
  d->default_connection = default_connection;
  return d;
}

void destroy_port_declaration(/*@ only @*/ port_declaration *d)
  /*@modifies d @*/ {
  xmlFree((xmlChar *)d->name);
  destroy_port_connection(d->default_connection);
  xmlFree(d);
}

static void port_declaration_deallocator(/*@ only @*/ xmlLinkPtr l) {
  destroy_port_source(xmlLinkGetData(l));
}

static int port_declaration_search_name(const void *data0,
                                        const void *data1) {
  return xmlStrcmp(((const port_declaration *)data0)->name,
                   ((const port_declaration *)data1)->name);
}

static void xml_doc_deallocator(xmlLinkPtr l) {
  xmlFreeDoc(xmlLinkGetData(l));
}

/*@ only @*/
input_port_instance *new_input_port_instance(/*@ dependent @*/ pipeline_step *owner,
                                             /*@ dependent @*/ port_declaration *decl,
                                             /*@ owned @*/ /*@ null @*/ 
                                             port_connection *connection) {
  input_port_instance *p = xmlMalloc(sizeof(*p));
  p->owner = owner;
  p->decl  = decl;
  p->queue = xmlListCreate(xml_doc_deallocator, NULL);
  p->complete = false;
  p->connection = connection;
  return p;
}

void destroy_input_port_instance(input_port_instance *p) {
  xmlListDelete(p->queue);
  destroy_port_connection(p->connection);
  xmlFree(p);
}

static void input_port_instance_deallocator(xmlLinkPtr l) {
  destroy_input_port_instance(xmlLinkGetData(l));
}

static int input_port_instance_search_name(const void *data0,
                                           const void *data1) {
  return xmlStrcmp(((const input_port_instance *)data0)->decl->name,
                   ((const input_port_instance *)data1)->decl->name);
}

pstep_option_decl *new_pstep_option_decl(const xmlChar *name, 
                                         const xmlChar *defval) {
  pstep_option_decl *pd = xmlMalloc(sizeof(*pd));

  if (defval == NULL) {
    pd->defval = NULL;
  } else {
    pd->defval = xmlXPathCompile(defval);
    if (!pd->defval) {
      xmlFree(pd);
      return NULL;
    }
  }
  pd->name = xmlStrdup(name);
  return pd;
}

void destroy_pstep_option_decl(pstep_option_decl *pd) {
  if (pd == NULL)
    return;
  xmlFree((xmlChar *)pd->name);
  if (pd->defval)
    xmlXPathFreeCompExpr(pd->defval);
  xmlFree(pd);
}

static void pstep_option_decl_deallocator(xmlLinkPtr l) {
  destroy_pstep_option_decl(xmlLinkGetData(l));
}

static int pstep_option_decl_search_name(const void *data0,
                                         const void *data1) {
  return xmlStrcmp(((const pstep_option_decl *)data0)->name,
                   ((const pstep_option_decl *)data1)->name);
}

pipeline_library *new_pipeline_library(const xmlChar *uri) {
  pipeline_library *l = xmlMalloc(sizeof(*l));
  l->uri = xmlStrdup(uri);
  l->types = xmlHashCreate(16);
  l->pipelines = xmlHashCreate(32);
  l->dyn_library = NULL;
  return l;
}

static void pipeline_decl_hash_deallocator(void *, xmlChar *);

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
  xmlHashFree(pl->types, pipeline_decl_hash_deallocator);
  xmlHashFree(pl->pipelines, pipeline_decl_hash_deallocator);
  xmlFree((xmlChar *)pl->uri);
  xmlFree(pl);
}

static void pipeline_step_deallocator(xmlLinkPtr l);
static int pipeline_step_search_name(const void *data0,
                                     const void *data1);

pipeline_decl *new_pipeline_decl(const xmlChar *ns,
                                 const xmlChar *name,
                                 const pipeline_atomic_type *type) {
  pipeline_decl *pd = xmlMalloc(sizeof(*pd));
  pd->ns = xmlStrdup(ns);
  pd->name = xmlStrdup(name);
  pd->type = type;
  pd->options = xmlListCreate(pstep_option_decl_deallocator,
                              pstep_option_decl_search_name);
  pd->ports = xmlListCreate(port_declaration_deallocator,
                            port_declaration_search_name);
  pd->body = xmlListCreate(pipeline_step_deallocator, 
                           pipeline_step_search_name);
  pd->imports = xmlListCreate(NULL, NULL);
  return pd;
}

void destroy_pipeline_decl(pipeline_decl *pd) {
  if (pd == NULL)
    return;
  
  xmlFree((void *)pd->ns);
  xmlFree((void *)pd->name);
  xmlListDelete(pd->options);
  xmlListDelete(pd->ports);
  xmlListDelete(pd->body);
  xmlListDelete(pd->imports);
}

static void pipeline_decl_hash_deallocator(void *data, 
                                           xmlChar *name ATTRIBUTE_UNUSED) {
  destroy_pipeline_decl(data);
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

static void pipeline_option_deallocator(xmlLinkPtr l) {
  destroy_pipeline_option(xmlLinkGetData(l));
}

static int pipeline_option_search_name(const void *data0,
                                       const void *data1) {
  return xmlStrcmp(((const pipeline_option *)data0)->decl->name,
                   ((const pipeline_option *)data1)->decl->name);
}

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
  xmlFree((void *)pa->ns);
  xmlFree((void *)pa->name);
  destroy_port_connection(pa->source);
  xmlFree(pa);
}

static void pipeline_assignment_deallocator(xmlLinkPtr l) {
  destroy_pipeline_assignment(xmlLinkGetData(l));
}

pipeline_branch *new_pipeline_branch(port_connection *test) {
  pipeline_branch *pb = xmlMalloc(sizeof(*pb));
  pb->test = test;
  pb->body = xmlListCreate(pipeline_step_deallocator, NULL);
  return pb;
}

void destroy_pipeline_branch(pipeline_branch *br) {
  if (!br)
    return;
  destroy_port_connection(br->test);
  xmlListDelete(br->body);
  xmlFree(br);
  
}

static void pipeline_branch_deallocator(xmlLinkPtr l) {
  destroy_pipeline_branch(xmlLinkGetData(l));
}

pipeline_step *new_pipeline_step(enum pipeline_step_kind kind, xmlChar *name, ...) {
  pipeline_step *ps = xmlMalloc(sizeof(*ps));
  va_list args;
  
  va_start(args, name);
  ps->kind = kind;
  ps->name = xmlStrdup(name);
  ps->inputs = xmlListCreate(input_port_instance_deallocator,
                             input_port_instance_search_name);
  ps->primary_inputs[0] = ps->primary_inputs[1] = NULL;
  ps->outputs = xmlListCreate(output_port_instance_deallocator,
                              output_port_instance_search_name);
  ps->primary_output = NULL;
  ps->options = xmlListCreate(pipeline_option_deallocator,
                              pipeline_option_search_name);
  switch (kind) {
    case PSTEP_ATOMIC:
      ps->x.atomic.decl = va_arg(args, pipeline_decl *);
      ps->x.atomic.data = NULL;
      break;
    case PSTEP_CALL:
      ps->x.call = va_arg(args, pipeline_decl *);
      break;
    case PSTEP_ASSIGN:
      ps->x.assign = va_arg(args, pipeline_assignment *);
      break;
    case PSTEP_FOREACH:
    case PSTEP_GROUP:
      ps->x.body = xmlListCreate(pipeline_step_deallocator, 
                                 pipeline_step_search_name);
      break;
    case PSTEP_VIEWPORT: 
    {
      const xmlChar *match = va_arg(args, const xmlChar *);
      if (!match)
        ps->x.viewport.match = NULL;
      else {
        ps->x.viewport.match = xmlXPathCompile(match);
        if (!ps->x.viewport.match) {
          xmlFree((void *)ps->name);
          xmlListDelete(ps->inputs);
          xmlListDelete(ps->outputs);
          xmlListDelete(ps->options);
          return NULL;
        }
      }
      ps->x.viewport.body = xmlListCreate(pipeline_step_deallocator,
                                          pipeline_step_search_name);
      
    }
    case PSTEP_CHOOSE:
      ps->x.choose.assign = xmlListCreate(pipeline_assignment_deallocator,
                                          NULL);
      ps->x.choose.branches = xmlListCreate(pipeline_branch_deallocator,
                                            NULL);
      ps->x.choose.otherwise = xmlListCreate(pipeline_step_deallocator,
                                             pipeline_step_search_name);
      break;
    case PSTEP_TRY:
      ps->x.trycatch.assign = xmlListCreate(pipeline_assignment_deallocator,
                                            NULL);
      ps->x.trycatch.try = va_arg(args, pipeline_step *);
      ps->x.trycatch.catch = va_arg(args, pipeline_step *);
    default:
      assert(0);
  }
  va_end(args);
  return ps;
}

void destroy_pipeline_step(pipeline_step *step) {
  if (!step)
    return;
  
  switch (step->kind) {
    case PSTEP_ATOMIC:
      if (step->x.atomic.data)
        step->x.atomic.decl->type->destroy(step);
      break;
    case PSTEP_CALL:
      break;
    case PSTEP_ASSIGN:
      destroy_pipeline_assignment(step->x.assign);
      break;
    case PSTEP_FOREACH:
    case PSTEP_GROUP:
      xmlListDelete(step->x.body);
      break;
    case PSTEP_VIEWPORT:
      if (step->x.viewport.match)
        xmlXPathFreeCompExpr(step->x.viewport.match);
      xmlListDelete(step->x.viewport.body);
      break;
    case PSTEP_CHOOSE:
      xmlListDelete(step->x.choose.assign);
      xmlListDelete(step->x.choose.branches);
      xmlListDelete(step->x.choose.otherwise);
      break;
    case PSTEP_TRY:
      xmlListDelete(step->x.trycatch.assign);
      destroy_pipeline_step(step->x.trycatch.try);
      destroy_pipeline_step(step->x.trycatch.catch);
      break;
    default:
      assert(0);
  }
  xmlListDelete(step->inputs);
  xmlListDelete(step->outputs);
  xmlListDelete(step->options);
  xmlFree((void *)step->name);
  xmlFree(step);
}

static void pipeline_step_deallocator(xmlLinkPtr l) {
  destroy_pipeline_step(xmlLinkGetData(l));
}

static int pipeline_step_search_name(const void *data0,
                                     const void *data1) {
  return xmlStrcmp(((const pipeline_step *)data0)->name,
                   ((const pipeline_step *)data1)->name);
}
