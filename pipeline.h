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
 * @brief pipeline data structures
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef PIPELINE_H
#define PIPELINE_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <libxml/list.h>
#include <libxml/hash.h>
#include <libxml/xpath.h>
#include <libxml/xmlmodule.h>

#define W3C_XPROC_NAMESPACE "http://www.w3.org/ns/xproc"
#define W3C_XPROC_STEP_NAMESPACE "http://www.w3.org/ns/xproc-step"
#define W3C_XPROC_ERROR_NAMESPACE "http://www.w3.org/ns/xproc-error"

enum port_source_kind {
  PORT_SOURCE_INLINE,
  PORT_SOURCE_DOCUMENT,
  PORT_SOURCE_DATA,
  PORT_SOURCE_PIPE,
  PORT_SOURCE_UNRESOLVED
} port_source_kind;

typedef struct output_port_instance {
  /*@ dependent @*/ struct pipeline_step *owner;
  /*@ dependent @*/ struct port_declaration *decl;
  /*@ dependent @*/ /*@ null @*/ struct input_port_instance *connected;
} output_port_instance;

typedef struct port_source_info {
  /*@ owned @*/ const xmlChar *uri;
  /*@ owned @*/ /*@ null @*/ const xmlChar *wrapper;
  /*@ owned @*/ /*@ null @*/ const xmlChar *wrapper_ns;
  /*@ owned @*/ /*@ null @*/ const xmlChar *content_type;
} port_source_info;

typedef struct port_source {
  enum port_source_kind kind;
  union {
    port_source_info load;
    xmlDocPtr doc;
    output_port_instance *pipe;
    struct {
      const xmlChar *step;
      const xmlChar *port;
    } ref;
  } x;
} port_source;

typedef struct port_connection {
  /*@refs@*/ unsigned refcnt;
  /*@null@*/ /*@owned@*/ xmlXPathCompExprPtr filter;
  xmlListPtr /* port_source */ sources; 
} port_connection;

typedef /*@refcounted@*/ port_connection *port_connection_ptr;

/*@unused@*/ /*@null@*/ /*@ newref @*/
static inline port_connection_ptr
use_port_connection(/*@ returned @*/ /*@ null @*/ port_connection_ptr p) {
  if (p)
    p->refcnt++;
  return p;
}

enum port_kind {
  PORT_INPUT_DOCUMENT,
  PORT_INPUT_PARAMETERS,
  PORT_OUTPUT,
  PORT_N_KINDS
};

typedef struct port_declaration {
  enum port_kind kind;
  const xmlChar *name;
  bool sequence;
  bool primary;
  port_connection *default_connection;
} port_declaration;

typedef struct input_port_instance {
  /*@ dependent @*/ struct pipeline_step *owner;
  /*@ dependent @*/ struct port_declaration *decl;
  /*@ owned @*/ xmlListPtr /* xmlDocPtr */ queue;
  bool complete;
  /*@ owned @*/ /*@ null @*/ port_connection *connection;
} input_port_instance;

typedef struct pstep_option_decl {
  const xmlChar *name;
  xmlXPathCompExprPtr defval;
} pstep_option_decl;

typedef struct pipeline_library {
  const xmlChar *uri;
  xmlHashTablePtr /* pipeline_decl */types;
  xmlHashTablePtr /* pipeline_decl */pipelines;
  xmlModulePtr dyn_library;
} pipeline_library;

struct pipeline_step;
struct pipeline_exec_context;
typedef bool (*pstep_instantiate_func)(struct pipeline_step *newstep);
typedef bool (*pstep_available_func)(const struct pipeline_step *newstep);
typedef void (*pstep_destroy_func)(struct pipeline_step *step);
typedef bool (*pstep_execute_func)(struct pipeline_step *step, 
                                   struct pipeline_exec_context *context);

typedef struct pipeline_atomic_type {
  pstep_instantiate_func instantiate;
  pstep_destroy_func destroy;
  pstep_available_func available;
  pstep_execute_func execute;
} pipeline_atomic_type;

typedef struct pipeline_decl {
  const xmlChar *ns;
  const xmlChar *name;
  const pipeline_atomic_type *type;
  xmlListPtr /* pstep_option_decl */ options;
  xmlListPtr /* port_declaration */ ports;
  port_declaration *primaries[PORT_N_KINDS];
  xmlListPtr /* pipeline_step */ body;
  xmlListPtr /* pipeline_library* */ imports;
} pipeline_decl;

extern pipeline_decl *pipeline_lookup_type(const pipeline_decl *source, 
                                           const xmlChar *ns, const xmlChar *name);

typedef struct pipeline_option {
  pstep_option_decl *decl;
  port_connection *value;
} pipeline_option;

enum pipeline_step_kind {
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
  const xmlChar *ns;
  const xmlChar *name;
  port_connection *source;
} pipeline_assignment;

typedef struct pipeline_branch {
  port_connection *test;
  xmlListPtr /* pipeline_step */ body;
} pipeline_branch;

typedef struct pipeline_step {
  const xmlChar *name;
  enum pipeline_step_kind kind;
  xmlListPtr /* input_port_instance */ inputs;
  input_port_instance *primary_inputs[2];
  xmlListPtr /* output_port_instance */ outputs;
  output_port_instance *primary_output;
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

extern void pipeline_run_hooks(const char *name, void *data);

extern void pipeline_xpath_update_doc(xmlXPathContextPtr ctx, 
                                      /*@ null @*/ /*@ dependent @*/ xmlDocPtr doc);
extern void pipeline_xpath_update_context(xmlXPathContextPtr ctx, 
                                          /*@ dependent @*/ pipeline_exec_context *pctxt);
extern /*@ null @*/ xmlXPathContext *pipeline_xpath_create_static_context(xmlDocPtr xproc_doc, 
                                                                          xmlNodePtr xproc_node);

extern char xproc_episode[];

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PIPELINE_H */
