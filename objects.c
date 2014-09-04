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

/*@ only @*/
output_port_instance *output_port_instance_new(/*@ dependent @*/ const port_declaration *decl,
                                               /*@ null @*/ /*@ dependent @*/ input_port_instance *connected) {
  output_port_instance *p = xmlMalloc(sizeof(*p));

  p->owner     = NULL;
  p->decl      = decl;
  p->connected = connected;
  return p;
}

void output_port_instance_destroy(/*@ null @*/ /*@ only @*/ output_port_instance *p) {
  xmlFree(p);
}

static void output_port_instance_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  output_port_instance_destroy(xmlLinkGetData(l));
}

static int output_port_instance_search_name(const void *data0,
                                            const void *data1) {
  return xmlStrcmp(((const output_port_instance *)data0)->decl->name,
                   ((const output_port_instance *)data1)->decl->name);
}

/*@ only @*/
port_source *port_source_new(enum port_source_kind kind, ...) {
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

void port_source_destroy(/*@null@*/ /*@only@*/port_source *s) 
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
      abort();
  }
  xmlFree(s);
}

static void port_source_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  port_source_destroy(xmlLinkGetData(l));
}

/*@ newref @*/
port_connection_ptr port_connection_new(const xmlChar *filter) {
  port_connection *p = xmlMalloc(sizeof(*p));

  p->refcnt = 1u;
  if (filter == NULL)
    p->filter = NULL;
  else {
    p->filter = xmlXPathCompile(filter);
    if (p->filter == NULL) {
      xmlFree(p);
      pipeline_report_xml_error(xmlGetLastError());
    }
  }
  p->sources = xmlListCreate(port_source_deallocator, NULL);
  return p;
}

void port_connection_destroy(/*@ null @*/ /*@ killref @*/ port_connection_ptr p) 
  /*@modifies p @*/ {
  /*@-mustfreeonly@*/
  if (p == NULL)
    return;
  assert(p->refcnt != 0);
  if (--p->refcnt > 0)
    return;
  /*@=mustfreeonly@*/
  
  if (p->filter != NULL)
    xmlXPathFreeCompExpr(p->filter);
  xmlListDelete(p->sources);
  /*@-refcounttrans@*/
  xmlFree(p);
  /*@=refcounttrans@*/
}

/*@ only @*/
port_declaration *port_declaration_new(enum port_kind kind,
                                       const xmlChar *name,
                                       bool sequence,
                                       bool primary,
                                       /*@ null @*/ port_connection_ptr default_connection) {
  port_declaration *d = xmlMalloc(sizeof(*d));
  d->kind = kind;
  d->name = xmlStrdup(name);
  d->sequence = sequence;
  d->primary = primary;
  d->default_connection = port_connection_use(default_connection);
  return d;
}

void port_declaration_destroy(/*@ only @*/ port_declaration *d)
  /*@modifies d @*/ {
  xmlFree((xmlChar *)d->name);
  port_connection_destroy(d->default_connection);
  xmlFree(d);
}

static void port_declaration_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  port_source_destroy(xmlLinkGetData(l));
}

static int port_declaration_search_name(const void *data0,
                                        const void *data1) {
  return xmlStrcmp(((const port_declaration *)data0)->name,
                   ((const port_declaration *)data1)->name);
}

/*@ only @*/
input_port_instance *input_port_instance_new(/*@ dependent @*/ const port_declaration *decl,
                                             /*@ null @*/ 
                                             port_connection_ptr connection) {
  input_port_instance *p = xmlMalloc(sizeof(*p));
  p->owner = NULL;
  p->decl  = decl;
  p->queue = xmlListCreate(NULL, NULL);
  p->complete = false;
  p->connection = port_connection_use(connection);
  return p;
}

void input_port_instance_destroy(/*@ only @*/ /*@ null @*/ input_port_instance *p) {
  if (p == NULL)
    return;
  
  for (;;) {
    xmlDocPtr next = input_port_instance_next_document(p);
    if (next == NULL)
      break;
    xmlFreeDoc(next);
  }
  xmlListDelete(p->queue);
  port_connection_destroy(p->connection);
  xmlFree(p);
}

static void input_port_instance_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  input_port_instance_destroy(xmlLinkGetData(l));
}

static int input_port_instance_search_name(const void *data0,
                                           const void *data1) {
  return xmlStrcmp(((const input_port_instance *)data0)->decl->name,
                   ((const input_port_instance *)data1)->decl->name);
}

/*@ only @*/
pstep_option_decl *pstep_option_decl_new(const xmlChar *name, 
                                         /*@ null @*/ const xmlChar *defval) {
  pstep_option_decl *pd = xmlMalloc(sizeof(*pd));

  if (defval == NULL) {
    pd->defval = NULL;
  } else {
    pd->defval = xmlXPathCompile(defval);
    if (pd->defval == NULL) {
      xmlFree(pd);
      pipeline_report_xml_error(xmlGetLastError());
    }
  }
  pd->name = xmlStrdup(name);
  return pd;
}

void pstep_option_decl_destroy(/*@ only @*/ /*@ null @*/ pstep_option_decl *pd) {
  if (pd == NULL)
    return;
  xmlFree((xmlChar *)pd->name);
  if (pd->defval != NULL)
    xmlXPathFreeCompExpr(pd->defval);
  xmlFree(pd);
}

static void pstep_option_decl_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  pstep_option_decl_destroy(xmlLinkGetData(l));
}

static int pstep_option_decl_search_name(const void *data0,
                                         const void *data1) {
  return xmlStrcmp(((const pstep_option_decl *)data0)->name,
                   ((const pstep_option_decl *)data1)->name);
}

pipeline_library *pipeline_library_new(const xmlChar *uri) {
  pipeline_library *l = xmlMalloc(sizeof(*l));
  l->uri = xmlStrdup(uri);
  l->types = xmlHashCreate(16);
  l->pipelines = xmlHashCreate(32);
  l->dyn_library = NULL;
  return l;
}

static void pipeline_decl_hash_deallocator(/*@ owned @*/ void *, xmlChar *);

void pipeline_library_destroy(/*@ only @*/ /*@ null @*/ pipeline_library *pl) {
  if (pl == NULL)
    return;
  
  if (pl->dyn_library != NULL) {
    void *hook = NULL;
    if (xmlModuleSymbol(pl->dyn_library, "on_library_destroy", &hook) == 0 &&
        hook != NULL) {
      /*@-castfcnptr@*/
      ((void (*)(pipeline_library *))hook)(pl);
      /*@=castfcnptr@*/
    }
    (void)xmlModuleClose(pl->dyn_library);
  }
  xmlHashFree(pl->types, pipeline_decl_hash_deallocator);
  xmlHashFree(pl->pipelines, pipeline_decl_hash_deallocator);
  xmlFree((xmlChar *)pl->uri);
  xmlFree(pl);
}

static void pipeline_step_deallocator(/*@ owned @*/ xmlLinkPtr l);
static int pipeline_step_search_name(const void *data0,
                                     const void *data1);

pipeline_decl *pipeline_decl_new(const xmlChar *ns,
                                 const xmlChar *name,
                                 /*@ dependent @*/ /*@null@*/ 
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
  pd->primaries[0] = NULL;
  pd->primaries[1] = NULL;
  pd->primaries[2] = NULL;
  pd->names = xmlHashCreate(16);
  return pd;
}

void pipeline_decl_destroy(/*@ only @*/ /*@ null @*/ pipeline_decl *pd) {
  if (pd == NULL)
    return;
  
  xmlFree((void *)pd->ns);
  xmlFree((void *)pd->name);
  xmlListDelete(pd->options);
  xmlListDelete(pd->ports);
  xmlListDelete(pd->body);
  xmlListDelete(pd->imports);
  xmlHashFree(pd->names, NULL);
  xmlFree(pd);
}

static void pipeline_decl_hash_deallocator(/*@ owned @*/ void *data, 
                                           /*@ unused @*/ xmlChar *name ATTRIBUTE_UNUSED) {
  pipeline_decl_destroy(data);
}


/*@ only @*/
pipeline_option *pipeline_option_new(/*@ dependent @*/ const pstep_option_decl *decl,
                                     port_connection_ptr value) {
  pipeline_option *po = xmlMalloc(sizeof(*po));
  po->decl = decl;
  po->value = port_connection_use(value);
  return po;
}

void pipeline_option_destroy(/*@ only @*/ /*@ null @*/ pipeline_option *po) {
  if (po == NULL)
    return;
  port_connection_destroy(po->value);
  xmlFree(po);
}

static void pipeline_option_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  pipeline_option_destroy(xmlLinkGetData(l));
}

static int pipeline_option_search_name(const void *data0,
                                       const void *data1) {
  return xmlStrcmp(((const pipeline_option *)data0)->decl->name,
                   ((const pipeline_option *)data1)->decl->name);
}

/*@ only @*/
pipeline_assignment *pipeline_assignment_new(const xmlChar *ns,
                                             const xmlChar *name,
                                             port_connection_ptr source) {
  pipeline_assignment *pa = xmlMalloc(sizeof(*pa));
  pa->ns = xmlStrdup(ns);
  pa->name = xmlStrdup(name);
  pa->source = port_connection_nonull_use(source);
  return pa;
}

void pipeline_assignment_destroy(/*@ only @*/ /*@ null @*/ pipeline_assignment *pa) {
  if (pa == NULL)
    return;
  xmlFree((void *)pa->ns);
  xmlFree((void *)pa->name);
  port_connection_destroy(pa->source);
  xmlFree(pa);
}

static void pipeline_assignment_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  pipeline_assignment_destroy(xmlLinkGetData(l));
}

/*@ only @*/
pipeline_branch *pipeline_branch_new(port_connection_ptr test) {
  pipeline_branch *pb = xmlMalloc(sizeof(*pb));
  pb->test = port_connection_nonull_use(test);
  pb->body = xmlListCreate(pipeline_step_deallocator, NULL);
  return pb;
}

void pipeline_branch_destroy(/*@ only @*/ /*@ null @*/ pipeline_branch *br) {
  if (br == NULL)
    return;
  port_connection_destroy(br->test);
  xmlListDelete(br->body);
  xmlFree(br);
  
}

static void pipeline_branch_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  pipeline_branch_destroy(xmlLinkGetData(l));
}

/*@ only @*/
pipeline_step *pipeline_step_new(enum pipeline_step_kind kind, const xmlChar *name, ...) {
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
    case PSTEP_ERROR:
      ps->x.error_code = va_arg(args, xproc_error);
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
      if (match == NULL)
        ps->x.viewport.match = NULL;
      else {
        ps->x.viewport.match = xmlXPathCompile(match);
        if (ps->x.viewport.match == NULL) {
          xmlFree((void *)ps->name);
          xmlListDelete(ps->inputs);
          xmlListDelete(ps->outputs);
          xmlListDelete(ps->options);
          xmlFree(ps);
          pipeline_report_xml_error(xmlGetLastError());
        }
      }
      ps->x.viewport.body = xmlListCreate(pipeline_step_deallocator,
                                          pipeline_step_search_name);
      break;
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
      break;
    default:
      abort();
  }
  va_end(args);
  return ps;
}

void pipeline_step_destroy(/*@ only @*/ /*@ null @*/ pipeline_step *step) {
  if (step == NULL)
    return;
  
  switch (step->kind) {
    case PSTEP_ATOMIC:
      if (step->x.atomic.data != NULL && step->x.atomic.decl->type != NULL)
        step->x.atomic.decl->type->destroy(step);
      break;
    case PSTEP_CALL:
      break;
    case PSTEP_ASSIGN:
      pipeline_assignment_destroy(step->x.assign);
      break;
    case PSTEP_FOREACH:
    case PSTEP_GROUP:
      xmlListDelete(step->x.body);
      break;
    case PSTEP_VIEWPORT:
      if (step->x.viewport.match != NULL)
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
      pipeline_step_destroy(step->x.trycatch.try);
      pipeline_step_destroy(step->x.trycatch.catch);
      break;
    default:
      abort();
  }
  xmlListDelete(step->inputs);
  xmlListDelete(step->outputs);
  xmlListDelete(step->options);
  xmlFree((void *)step->name);
  xmlFree(step);
}

static void pipeline_step_deallocator(/*@ owned @*/ xmlLinkPtr l) {
  pipeline_step_destroy(xmlLinkGetData(l));
}

static int pipeline_step_search_name(const void *data0,
                                     const void *data1) {
  return xmlStrcmp(((const pipeline_step *)data0)->name,
                   ((const pipeline_step *)data1)->name);
}

void pipeline_library_add_type(pipeline_library *lib,
                               pipeline_decl *decl) {
  if (xmlHashAddEntry2(lib->types,
                       decl->name == NULL ? 
                       (const xmlChar *)"" : decl->name,
                       decl->ns,
                       decl) == 0)
    return;
  pipeline_report_xproc_error(NULL,
                              decl->name,
                              decl->ns,
                              XPROC_ERR_STATIC_DUPLICATE_TYPE,
                              lib->uri,
                              0, 0, 0);
}

void pipeline_library_add_pipeline(pipeline_library *lib,
                                   pipeline_decl *decl) {
  if (xmlHashAddEntry(lib->pipelines,
                      decl->name == NULL ? 
                      (const xmlChar *)"" : decl->name,
                      decl) == 0)
    return;
  pipeline_report_xproc_error(decl->name,
                              NULL, NULL,
                              XPROC_ERR_STATIC_DUPLICATE_STEP_NAME,
                              lib->uri,
                              0, 0, 0);
}

void port_connection_add_source(port_connection *pc,
                                port_source *ps) {
  xmlListPushBack(pc->sources, ps);
}

void input_port_instance_push_document(input_port_instance *ipi,
                                       xmlDocPtr doc) {
  xmlListPushFront(ipi->queue, doc);
}


xmlDocPtr input_port_instance_next_document(input_port_instance *ipi) {
  xmlLinkPtr front = xmlListFront(ipi->queue);
  xmlDocPtr doc = NULL;
  
  if (front == NULL)
    return NULL;
  doc = (xmlDocPtr)xmlLinkGetData(front);
  xmlListPopFront(ipi->queue);
  return doc;
}

void pipeline_decl_add_option(pipeline_decl *decl,
                              pstep_option_decl *option) {
  if (xmlListSearch(decl->options, option) != NULL) {
    pipeline_report_xproc_error(NULL, decl->name, decl->ns,
                                XPROC_ERR_STATIC_DUPLICATE_BINDING,
                                NULL, 0, 0, 0);
  }
  xmlListPushFront(decl->options, option);
}


void pipeline_decl_add_port(pipeline_decl *decl,
                            port_declaration *port) {
  /*@+enumindex@*/
  if (port->primary && decl->primaries[port->kind] != NULL) {
    pipeline_report_xproc_error(NULL, decl->name, decl->ns,
                                port->kind == PORT_OUTPUT ?
                                XPROC_ERR_STATIC_DUPLICATE_PRIMARY_OUTPUT :
                                XPROC_ERR_STATIC_DUPLICATE_PRIMARY_INPUT,
                                NULL, 0, 0, 0);    
  }
  if (xmlListSearch(decl->ports, port) != NULL) {
    pipeline_report_xproc_error(NULL, decl->name, decl->ns,
                                XPROC_ERR_STATIC_DUPLICATE_PORT_NAME,
                                NULL, 0, 0, 0);
  }
  xmlListPushFront(decl->ports, port);
  if (port->primary) 
    decl->primaries[port->kind] = port;
  /*@=enumindex@*/
}

static void generic_add_step(pipeline_decl *decl, xmlListPtr target, /*@keep@*/ pipeline_step *step)
  /*@modifies decl->names,target @*/ {
  if (step->name != NULL && 
      xmlHashLookup(decl->names, step->name) != NULL) {
    pipeline_report_xproc_error(step->name, NULL, NULL,
                                XPROC_ERR_STATIC_DUPLICATE_STEP_NAME,
                                NULL, 0, 0, 0);
  }
  xmlListPushBack(target, step);
  /*@-kepttrans@*/
  if (step->name != NULL)
    (void)xmlHashAddEntry(decl->names, step->name, step);
  /*@=kepttrans@*/
}

void pipeline_decl_add_step(pipeline_decl *decl,
                            pipeline_step *step) {
  generic_add_step(decl, decl->body, step);
  
}

void pipeline_decl_import(pipeline_decl *decl,
                          const xmlChar *uri) {
  pipeline_library *lib = pipeline_library_load(uri);
  /*@-dependenttrans@*/
  xmlListPushBack(decl->imports, lib);
  /*@=dependenttrans@*/
}

void pipeline_branch_add_step(pipeline_decl *decl,
                              pipeline_branch *branch,
                              pipeline_step *step) {
  generic_add_step(decl, branch->body, step);
}


/*@-mustmod@*/
void pipeline_step_add_step(pipeline_decl *decl,
                            pipeline_step *step,
                            pipeline_step *innerstep) {
  xmlListPtr target = NULL;

  switch (step->kind) {
    case PSTEP_FOREACH:
    case PSTEP_GROUP:
      target = step->x.body;
      break;
    case PSTEP_VIEWPORT:
      target = step->x.viewport.body;
      break;
    case PSTEP_CHOOSE:
      target = step->x.choose.otherwise;
      break;
    default:
      abort();
  }
  generic_add_step(decl, target, innerstep);
}
/*@=mustmod@*/


void pipeline_step_add_option(pipeline_step *step,
                              pipeline_option *option) {
  if (xmlListSearch(step->options, option) != NULL) {
    pipeline_report_xproc_error(step->name, NULL, NULL,
                                XPROC_ERR_STATIC_DUPLICATE_BINDING,
                                NULL, 0, 0, 0);
  }
  xmlListPushFront(step->options, option);
}

void pipeline_step_add_input_port(pipeline_step *step,
                                  input_port_instance *inp) {
  assert(inp->owner == NULL);
  if (inp->decl->primary && 
      step->primary_inputs[inp->decl->kind == PORT_INPUT_PARAMETERS] != NULL) {
    pipeline_report_xproc_error(step->name, NULL, NULL,
                                XPROC_ERR_STATIC_DUPLICATE_PRIMARY_INPUT,
                                NULL, 0, 0, 0);    
  }
  if (xmlListSearch(step->inputs, inp) != NULL) {
    pipeline_report_xproc_error(step->name, NULL, NULL,
                                XPROC_ERR_STATIC_DUPLICATE_PORT_NAME,
                                NULL, 0, 0, 0);
  }
  xmlListPushFront(step->inputs, inp);
  if (inp->decl->primary) 
    step->primary_inputs[inp->decl->kind == PORT_INPUT_PARAMETERS] = inp;
  inp->owner = step;
}

extern void pipeline_step_add_output_port(pipeline_step *step,
                                          output_port_instance *outp);

extern void pipeline_step_add_assignment(pipeline_step *step,
                                         pipeline_assignment *assign);
extern void pipeline_step_add_branch(pipeline_step *step,
                                     pipeline_branch *branch);

