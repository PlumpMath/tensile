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
  /*@ dependent @*/ const struct pipeline_step *owner;
  /*@ dependent @*/ const struct port_declaration *decl;
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
  /*@owned@*/ xmlListPtr /* port_source */ sources; 
} port_connection;

typedef /*@refcounted@*/ port_connection *port_connection_ptr;

static inline
/*@newref@*/
port_connection_ptr
use_port_connection_nonull(port_connection_ptr p)
  /*@modifies p @*/
{
  p->refcnt++;
  /*@-refcounttrans @*/
  return p;
  /*@=refcounttrans @*/
}

static inline
/*@unused@*/ /*@null@*/ /*@newref@*/
port_connection_ptr
use_port_connection(/*@null@*/ port_connection_ptr p) {
  if (p == NULL)
    return NULL;
  return use_port_connection_nonull(p);
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
  /*@ null @*/ port_connection_ptr default_connection;
} port_declaration;

typedef struct input_port_instance {
  /*@ dependent @*/ const struct pipeline_step *owner;
  /*@ dependent @*/ const struct port_declaration *decl;
  /*@ owned @*/ xmlListPtr /* xmlDocPtr */ queue;
  bool complete;
  /*@ null @*/ port_connection_ptr connection;
} input_port_instance;

typedef struct pstep_option_decl {
  /*@ owned @*/ const xmlChar *name;
  /*@ null @*/ /*@ owned @*/ xmlXPathCompExprPtr defval;
} pstep_option_decl;

typedef struct pipeline_library {
  const xmlChar *uri;
  xmlHashTablePtr /* pipeline_decl */types;
  xmlHashTablePtr /* pipeline_decl */pipelines;
  /*@ null @*/ xmlModulePtr dyn_library;
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

typedef /*@ null @*/ /*@ dependent @*/ port_declaration *port_declaration_aux_ptr;
typedef struct pipeline_decl {
  /*@ null @*/ /*@ owned @*/ const xmlChar *ns;
  /*@ null @*/ /*@ owned @*/ const xmlChar *name;
  /*@ null @*/ /*@ dependent @*/ const pipeline_atomic_type *type;
  xmlListPtr /* pstep_option_decl */ options;
  xmlListPtr /* port_declaration */ ports;
  port_declaration_aux_ptr primaries[PORT_N_KINDS];
  xmlListPtr /* pipeline_step */ body;
  xmlListPtr /* pipeline_library* */ imports;
} pipeline_decl;

extern pipeline_decl *pipeline_lookup_type(const pipeline_decl *source, 
                                           const xmlChar *ns, const xmlChar *name);

typedef struct pipeline_option {
  /*@ dependent @*/ const pstep_option_decl *decl;
  /*@ null @*/ port_connection_ptr value;
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
  /*@ owned @*/ const xmlChar *ns;
  /*@ owned @*/ const xmlChar *name;
  port_connection_ptr source;
} pipeline_assignment;

typedef struct pipeline_branch {
  port_connection_ptr test;
  /*@ owned @*/ xmlListPtr /* pipeline_step */ body;
} pipeline_branch;

typedef /*@ null @*/ /*@ dependent @*/ input_port_instance *input_port_instance_aux_ptr;
typedef /*@ null @*/ /*@ dependent @*/ output_port_instance *output_port_instance_aux_ptr;
typedef struct pipeline_step {
  /*@ owned @*/ const xmlChar *name;
  enum pipeline_step_kind kind;
  /*@ owned @*/ xmlListPtr /* input_port_instance */ inputs;
  input_port_instance_aux_ptr primary_inputs[2];
  /*@ owned @*/ xmlListPtr /* output_port_instance */ outputs;
  output_port_instance_aux_ptr primary_output;
  /*@ owned @*/ xmlListPtr /* pipeline_option */ options;
  union {
    struct {
      /*@ dependent @*/ const pipeline_decl *decl;
      /*@ owned @*/ /*@ null @*/ void *data;
    } atomic;
    /*@ dependent @*/ const pipeline_decl *call;
    /*@ owned @*/ pipeline_assignment *assign;
    /*@ owned @*/ xmlListPtr /* pipeline_step */ body;
    struct {
      /*@ owned @*/ xmlXPathCompExprPtr match;
      /*@ owned @*/ xmlListPtr /* pipeline_step */ body;
    } viewport;
    struct {
      /*@ owned @*/ xmlListPtr /* pipeline_assignment */ assign;
      /*@ owned @*/ xmlListPtr /* pipeline_branch */ branches;
      /*@ owned @*/ xmlListPtr /* pipeline_step */ otherwise;
    } choose;
    struct {
      /*@ owned @*/ xmlListPtr /* pipeline_assignment */ assign;
      /*@ owned @*/ struct pipeline_step *try;
      /*@ owned @*/ struct pipeline_step *catch;
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

extern /*@ only @*/
output_port_instance *new_output_port_instance(/*@ dependent @*/ pipeline_step *owner,
                                                      /*@ dependent @*/ port_declaration *decl,
                                                      /*@ null @*/ /*@ dependent @*/ 
                                                      input_port_instance *connected);

extern void destroy_output_port_instance(/*@ null @*/ /*@ only @*/ output_port_instance *p) /*@modifies p @*/;

extern /*@ only @*/ /*@ null @*/
port_source *new_port_source(enum port_source_kind kind, ...);

extern void destroy_port_source(/*@null@*/ /*@only@*/ port_source *s) 
  /*@modifies s @*/;

extern /*@ null @*/ /*@ newref @*/
port_connection_ptr new_port_connection(const xmlChar *filter);

extern void destroy_port_connection(/*@ null @*/ /*@ killref @*/ port_connection_ptr p) 
  /*@modifies p @*/;

extern /*@ only @*/
port_declaration *new_port_declaration(enum port_kind kind,
                                              const xmlChar *name,
                                              bool sequence,
                                              bool primary,
                                              /*@ null @*/ port_connection_ptr default_connection);

extern void destroy_port_declaration(/*@ only @*/ port_declaration *d)
  /*@modifies d @*/;

extern /*@ only @*/
input_port_instance *new_input_port_instance(/*@ dependent @*/ const pipeline_step *owner,
                                                    /*@ dependent @*/ const port_declaration *decl,
                                                    /*@ null @*/ 
                                                    port_connection_ptr connection);

extern void destroy_input_port_instance(/*@ only @*/ /*@ null @*/ input_port_instance *p);

extern /*@ only @*/ /*@ null @*/
pstep_option_decl *new_pstep_option_decl(const xmlChar *name, 
                                         /*@ null @*/ const xmlChar *defval);

extern void destroy_pstep_option_decl(/*@ only @*/ /*@ null @*/ pstep_option_decl *pd) 
  /*@modifies pd @*/;
extern /*@only@*/ pipeline_library *new_pipeline_library(const xmlChar *uri);
extern void destroy_pipeline_library(/*@ only @*/ /*@ null @*/ pipeline_library *pl)
  /*@modifies pl @*/;

extern /*@only@*/ pipeline_decl *new_pipeline_decl(const xmlChar *ns,
                                                   const xmlChar *name,
                                                   /*@ dependent @*/ /*@null@*/ 
                                                   const pipeline_atomic_type *type);
extern void destroy_pipeline_decl(/*@ only @*/ /*@ null @*/ pipeline_decl *pd)
  /*@modifies pd @*/;

extern /*@ only @*/
pipeline_option *new_pipeline_option(/*@ dependent @*/ const pstep_option_decl *decl,
                                     port_connection_ptr value);
extern void destroy_pipeline_option(/*@ only @*/ /*@ null @*/ pipeline_option *po);

extern /*@ only @*/
pipeline_assignment *new_pipeline_assignment(const xmlChar *ns,
                                             const xmlChar *name,
                                             port_connection_ptr source);

extern void destroy_pipeline_assignment(/*@ only @*/ /*@ null @*/ pipeline_assignment *pa)
  /*@modifies pa @*/;

extern /*@ only @*/ pipeline_branch *new_pipeline_branch(port_connection_ptr test);

extern void destroy_pipeline_branch(/*@ only @*/ /*@ null @*/ pipeline_branch *br);

extern /*@ null @*/ /*@ only @*/
pipeline_step *new_pipeline_step(enum pipeline_step_kind kind, const xmlChar *name, ...);

extern void destroy_pipeline_step(/*@ only @*/ /*@ null @*/ pipeline_step *step)
  /*@modifies step @*/;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PIPELINE_H */
