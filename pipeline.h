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
  /*@ dependent @*/ /*@null@*/ const struct pipeline_step *owner;
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
port_connection_nonull_use(port_connection_ptr p)
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
port_connection_use(/*@null@*/ port_connection_ptr p) {
  if (p == NULL)
    return NULL;
  return port_connection_nonull_use(p);
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
  /*@ dependent @*/ /*@null@*/ const struct pipeline_step *owner;
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
  xmlHashTablePtr /* pipeline_step */ names;
} pipeline_decl;

extern pipeline_decl *pipeline_lookup_type(const pipeline_decl *source, 
                                           const xmlChar *ns, const xmlChar *name);

typedef struct pipeline_option {
  /*@ dependent @*/ const pstep_option_decl *decl;
  /*@ null @*/ port_connection_ptr value;
} pipeline_option;

enum pipeline_step_kind {
  PSTEP_ATOMIC,
  PSTEP_ERROR,
  PSTEP_CALL,
  PSTEP_ASSIGN,
  PSTEP_FOREACH,
  PSTEP_VIEWPORT,
  PSTEP_CHOOSE,
  PSTEP_GROUP,
  PSTEP_TRY
};


typedef enum xproc_error {
  XPROC_ERR_STATIC_CONNECTION_LOOP = 1,
  XPROC_ERR_STATIC_DUPLICATE_STEP_NAME,
  XPROC_ERR_STATIC_UNCONNECTED_INPUT,
  XPROC_ERR_STATIC_DUPLICATE_BINDING,
  XPROC_ERR_STATIC_UNCONNECTED_PRIMARY_OUTPUT,
  XPROC_ERR_STATIC_NO_PRIMARY_OUTPUT,
  XPROC_ERR_STATIC_INCONSISTENT_OUTPUTS,
  XPROC_ERR_STATIC_INVALID_ATTRIBUTE,
  XPROC_ERR_STATIC_NONCONFORMING_STEP,
  XPROC_ERR_STATIC_DUPLICATE_PORT_NAME,
  XPROC_ERR_STATIC_DUPLICATE_PRIMARY_OUTPUT,
  XPROC_ERR_STATIC_EMPTY_COMPOUND,
  XPROC_ERR_STATIC_INCONSISTENT_OPTION_DECL,
  XPROC_ERR_STATIC_INVALID_VARIABLE_CONNECTION,
  XPROC_ERR_STATIC_INVALID_NAMESPACE,
  XPROC_ERR_STATIC_INVALID_PIPE,
  XPROC_ERR_STATIC_MALFORMED_INLINE,
  XPROC_ERR_STATIC_INVALID_TYPE_NS,
  XPROC_ERR_STATIC_INVALID_LOG,
  XPROC_ERR_STATIC_INCONSISTENT_OPTION,
  XPROC_ERR_STATIC_RESERVED_NS,
  XPROC_ERR_STATIC_CONNECTED_OUTPUT_IN_ATOMIC_STEP,
  XPROC_ERR_STATIC_DUPLICATE_PRIMARY_INPUT,
  XPROC_ERR_STATIC_UNDECLARED_OPTION,
  XPROC_ERR_STATIC_NO_DEFAULT_INPUT_PORT,
  XPROC_ERR_STATIC_INVALID_PORT_KIND,
  XPROC_ERR_STATIC_INVALID_PARAMETER_PORT,
  XPROC_ERR_STATIC_CONNECTED_PARAMETER_PORT,
  XPROC_ERR_STATIC_DUPLICATE_TYPE,
  XPROC_ERR_STATIC_MISPLACED_TEXT_NODE,
  XPROC_ERR_STATIC_MISSING_REQUIRED_ATTRIBUTE,
  XPROC_ERR_STATIC_INVALID_SERIALIZATION,
  XPROC_ERR_STATIC_PARAMETER_PORT_NOT_SEQUENCE,
  XPROC_ERR_STATIC_INCONSISTENT_NAMESPACE_BINDING,
  XPROC_ERR_STATIC_CONNECTED_INPUT_IN_ATOMIC_STEP,
  XPROC_ERR_STATIC_DECLARED_STEP_IS_COMPOUND,
  XPROC_ERR_STATIC_INVALID_EXCEPT_PREFIXES,
  XPROC_ERR_STATIC_CANNOT_IMPORT,
  XPROC_ERR_STATIC_IMPORT_WITHOUT_TYPE,
  XPROC_ERR_STATIC_UNCONNECTED_PARAMETER_INPUT,
  XPROC_ERR_STATIC_INVALID_EXCLUDE_INLINE_PREFIXES,
  XPROC_ERR_STATIC_NO_DEFAULT_FOR_EXCLUDE_INLINE_PREFIXES,
  XPROC_ERR_STATIC_INVALID_TOPLEVEL_ELEMENT,
  XPROC_ERR_STATIC_UNSUPPORTED_VERSION,
  XPROC_ERR_STATIC_NONSTATIC_USE_WHEN,
  XPROC_ERR_STATIC_UNDEFINED_VERSION,
  XPROC_ERR_STATIC_MALFORMED_VERSION,
  XPROC_ERR_DYNAMIC_NONXML_OUTPUT = 10001,
  XPROC_ERR_DYNAMIC_SEQUENCE_IN_VIEWPORT_SOURCE,
  XPROC_ERR_DYNAMIC_NOTHING_IS_CHOSEN,
  XPROC_ERR_DYNAMIC_SEQUENCE_IN_XPATH_CONTEXT,
  XPROC_ERR_DYNAMIC_UNEXPECTED_INPUT_SEQUENCE,
  XPROC_ERR_DYNAMIC_UNEXPECTED_OUTPUT_SEQUENCE, 
  XPROC_ERR_DYNAMIC_SEQUENCE_AS_CONTEXT_NODE,
  XPROC_ERR_DYNAMIC_INVALID_NAMESPACE_REF,
  XPROC_ERR_DYNAMIC_NO_VIEWPORT_MATCH,
  XPROC_ERR_DYNAMIC_INVALID_DOCUMENT,
  XPROC_ERR_DYNAMIC_UNSUPPORTED_URI_SCHEME,
  XPROC_ERR_DYNAMIC_INCONSISTENT_NAMESPACE_BINDING,
  XPROC_ERR_DYNAMIC_MALFORMED_PARAM,
  XPROC_ERR_DYNAMIC_UNRESOLVED_QNAME,
  XPROC_ERR_DYNAMIC_BAD_INPUT_SELECT,
  XPROC_ERR_DYNAMIC_UNSUPPORTED_STEP,
  XPROC_ERR_DYNAMIC_MALFORMED_PARAM_LIST,
  XPROC_ERR_DYNAMIC_INVALID_OPTION_TYPE,
  XPROC_ERR_DYNAMIC_INVALID_SERIALIZATION_OPTIONS,
  XPROC_ERR_DYNAMIC_ACCESS_DENIED,
  XPROC_ERR_DYNAMIC_NEED_PSVI,
  XPROC_ERR_DYNAMIC_BAD_XPATH,
  XPROC_ERR_DYNAMIC_NEED_XPATH10,
  XPROC_ERR_DYNAMIC_INVALID_PARAM_NAMESPACE,
  XPROC_ERR_DYNAMIC_NO_CONTEXT,
  XPROC_ERR_DYNAMIC_UNSUPPORTED_XPATH_VERSION,
  XPROC_ERR_DYNAMIC_INVALID_ATTRIBUTE_TYPE,
  XPROC_ERR_DYNAMIC_BAD_DATA,
  XPROC_ERR_DYNAMIC_STEP_FAILED,
  XPROC_ERR_DYNAMIC_RESERVED_NAMESPACE,
  XPROC_ERR_DYNAMIC_VALUE_NOT_AVAILABLE,
  XPROC_ERR_DYNAMIC_INCONSISTENT_NAMESPACE,
  XPROC_ERR_STEP_MALFORMED_MIME_BOUNDARY = 20001,
  XPROC_ERR_STEP_NO_AUTH_METHOD,
  XPROC_ERR_STEP_INVALID_STATUS_ONLY_FLAG,
  XPROC_ERR_STEP_ENTITY_BODY_NOT_ALLOWED,
  XPROC_ERR_STEP_NO_METHOD,
  XPROC_ERR_STEP_INVALID_CHARSET,
  XPROC_ERR_STEP_DIRECTORY_ACCESS_DENIED,
  XPROC_ERR_STEP_INCONSISTENT_RENAME,
  XPROC_ERR_STEP_RENAME_RESERVED_NS,
  XPROC_ERR_STEP_NOT_A_DIRECTORY,
  XPROC_ERR_STEP_DOCUMENTS_NOT_EQUAL,
  XPROC_ERR_STEP_INCONSISTENT_VALUES,
  XPROC_ERR_STEP_MALFORMED_BODY,
  XPROC_ERR_STEP_INVALID_NODE_TYPE,
  XPROC_ERR_STEP_INVALID_INSERT,
  XPROC_ERR_STEP_DTD_VALIDATION_NOT_SUPPORTED,
  XPROC_ERR_STEP_TAGS_IN_BODY,
  XPROC_ERR_STEP_XINCLUDE_ERROR,
  XPROC_ERR_STEP_INVALID_OVERRIDE_CONTENT_TYPE,
  XPROC_ERR_STEP_COMMAND_CANNOT_RUN,
  XPROC_ERR_STEP_INCONSISTENT_EXEC_WRAPPER,
  XPROC_ERR_STEP_NOT_URLENCODED,
  XPROC_ERR_STEP_UNSUPPORTED_XSLT_VERSION,
  XPROC_ERR_STEP_NOT_A_REQUEST,
  XPROC_ERR_STEP_CANNOT_STORE,
  XPROC_ERR_STEP_UNSUPPORTED_CONTENT_TYPE,
  XPROC_ERR_STEP_DOCUMENT_NOT_VALID,
  XPROC_ERR_STEP_SCHEMATRON_ASSERTION_FAIL,
  XPROC_ERR_STEP_UNSUPPORTED_VALIDATION_MODE,
  XPROC_ERR_STEP_CANNOT_APPLY_TEMPLATE,
  XPROC_ERR_STEP_MALFORMED_XQUERY_RESULT,
  XPROC_ERR_STEP_INVALID_ADD_ATTRIBUTE,
  XPROC_ERR_STEP_UNSUPPORTED_UUID_VERSION,
  XPROC_ERR_STEP_INVALID_NCNAME,
  XPROC_ERR_STEP_DELETE_NAMESPACE_NODE,
  XPROC_ERR_STEP_COMMAND_EXIT_FAILURE,
  XPROC_ERR_STEP_INVALID_ARG_SEPARATOR
} xproc_error;


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
    enum xproc_error error_code;
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


typedef struct pipeline_error_info {
  xmlChar *step_name;
  xmlChar *step_type;
  xmlChar *step_type_ns;
  xmlChar *code;
  xmlChar *code_ns;
  xmlChar *href;
  int lineno;
  int columno;
  int offset;
  xmlChar *details;
} pipeline_error_info;

typedef void (*pipeline_error_handler)(/*@only@*/ pipeline_error_info *err,
                                       /*@null@*/ void *data);

typedef void (*pipeline_guarded_func)(/*@null@*/ void *data);

extern void pipeline_run_with_error_handler(pipeline_guarded_func body,
                                            /*@null@*/ void *data,
                                            pipeline_error_handler handler,
                                            /*@null@*/ void *handler_data);

extern  /*@noreturn@*/
void pipeline_report_error(/*@null@*/ const xmlChar *step_name,
                           /*@null@*/ const xmlChar *step_type,
                           /*@null@*/ const xmlChar *step_type_ns,
                           const xmlChar *code,
                           /*@null@*/const xmlChar *code_ns,
                           /*@null@*/const xmlChar *href,
                           int lineno,
                           int columno,
                           int offset,
                           /*@null@*/const xmlChar *details);

extern /*@noreturn@*/
void pipeline_report_xproc_error(/*@null@*/ const xmlChar *step_name,
                                 /*@null@*/ const xmlChar *step_type,
                                 /*@null@*/ const xmlChar *step_type_ns,
                                 xproc_error code,
                                 /*@null@*/const xmlChar *href,
                                 int lineno,
                                 int columno,
                                 int offset);

extern /*@noreturn@*/ void pipeline_report_xml_error(/*@null@*/ xmlErrorPtr err);
extern /*@noreturn@*/ void pipeline_report_xpath_error(xmlXPathContextPtr ctx);

extern /*@noreturn@*/ void pipeline_escalate_error(/*@only@*/ pipeline_error_info *err);

typedef void (*pipeline_cleanup_func)(/*@only@*/ void *data);

extern void pipeline_cleanup_on_error(pipeline_cleanup_func func, 
                                      /*@keep@*/ void *data);

extern void pipeline_dequeue_cleanup(/*@dependent@*/ const void *data, bool perform);

typedef void (*pipeline_hook_func)(/*@null@*/ void *data);

extern void pipeline_run_hooks(const char *name, /*@null@*/void *data);

extern void pipeline_xpath_update_doc(xmlXPathContextPtr ctx, 
                                      /*@ null @*/ /*@ dependent @*/ xmlDocPtr doc);
extern void pipeline_xpath_update_context(xmlXPathContextPtr ctx, 
                                          /*@ dependent @*/ pipeline_exec_context *pctxt);
extern xmlXPathContext *pipeline_xpath_create_static_context(xmlDocPtr xproc_doc, 
                                                             xmlNodePtr xproc_node);

extern char xproc_episode[];

extern /*@ only @*/
output_port_instance *output_port_instance_new(/*@ dependent @*/ const port_declaration *decl,
                                               /*@ null @*/ /*@ dependent @*/ 
                                               input_port_instance *connected);

extern void output_port_instance_destroy(/*@ null @*/ /*@ only @*/ output_port_instance *p) /*@modifies p @*/;

extern /*@ only @*/ 
port_source *port_source_new(enum port_source_kind kind, ...);

extern void port_source_destroy(/*@null@*/ /*@only@*/ port_source *s) 
  /*@modifies s @*/;

extern /*@ newref @*/
port_connection_ptr port_connection_new(const xmlChar *filter);

extern void port_connection_destroy(/*@ null @*/ /*@ killref @*/ port_connection_ptr p) 
  /*@modifies p @*/;

extern /*@ only @*/
port_declaration *port_declaration_new(enum port_kind kind,
                                              const xmlChar *name,
                                              bool sequence,
                                              bool primary,
                                              /*@ null @*/ port_connection_ptr default_connection);

extern void port_declaration_destroy(/*@ only @*/ port_declaration *d)
  /*@modifies d @*/;

extern /*@ only @*/
input_port_instance *input_port_instance_new(/*@ dependent @*/ const port_declaration *decl,
                                             /*@ null @*/ 
                                             port_connection_ptr connection);

extern void input_port_instance_destroy(/*@ only @*/ /*@ null @*/ input_port_instance *p);

extern /*@ only @*/
pstep_option_decl *pstep_option_decl_new(const xmlChar *name, 
                                         /*@ null @*/ const xmlChar *defval);

extern void pstep_option_decl_destroy(/*@ only @*/ /*@ null @*/ pstep_option_decl *pd) 
  /*@modifies pd @*/;
extern /*@only@*/ pipeline_library *pipeline_library_new(const xmlChar *uri);
extern void pipeline_library_destroy(/*@ only @*/ /*@ null @*/ pipeline_library *pl)
  /*@modifies pl @*/;

extern /*@only@*/ pipeline_decl *pipeline_decl_new(const xmlChar *ns,
                                                   const xmlChar *name,
                                                   /*@ dependent @*/ /*@null@*/ 
                                                   const pipeline_atomic_type *type);
extern void pipeline_decl_destroy(/*@ only @*/ /*@ null @*/ pipeline_decl *pd)
  /*@modifies pd @*/;

extern /*@ only @*/
pipeline_option *pipeline_option_new(/*@ dependent @*/ const pstep_option_decl *decl,
                                     port_connection_ptr value);
extern void pipeline_option_destroy(/*@ only @*/ /*@ null @*/ pipeline_option *po);

extern /*@ only @*/
pipeline_assignment *pipeline_assignment_new(const xmlChar *ns,
                                             const xmlChar *name,
                                             port_connection_ptr source);

extern void pipeline_assignment_destroy(/*@ only @*/ /*@ null @*/ pipeline_assignment *pa)
  /*@modifies pa @*/;

extern /*@ only @*/ pipeline_branch *pipeline_branch_new(port_connection_ptr test);

extern void pipeline_branch_destroy(/*@ only @*/ /*@ null @*/ pipeline_branch *br);

extern /*@ only @*/
pipeline_step *pipeline_step_new(enum pipeline_step_kind kind, const xmlChar *name, ...);

extern void pipeline_step_destroy(/*@ only @*/ /*@ null @*/ pipeline_step *step)
  /*@modifies step @*/;

extern void pipeline_library_add_type(pipeline_library *lib,
                                      /*@ keep @*/
                                      pipeline_decl *decl)
  /*@modifies lib->types @*/;

extern void pipeline_library_add_pipeline(pipeline_library *lib,
                                          /*@ keep @*/
                                          pipeline_decl *decl)
  /*@modifies lib->pipelines @*/;

extern void port_connection_add_source(port_connection *pc,
                                /*@keep@*/
                                port_source *ps)
  /*@modifies pc->sources @*/;

extern void input_port_instance_push_document(input_port_instance *ipi,
                                              /*@keep@*/
                                              xmlDocPtr doc)
  /*@modifies ipi->queue @*/;

extern /*@only@*/ /*@null@*/
xmlDocPtr input_port_instance_next_document(input_port_instance *ipi);


extern /*@dependent@*/ pipeline_library *pipeline_library_load(const xmlChar *uri);


extern void pipeline_decl_add_option(pipeline_decl *decl,
                                     /*@keep@*/ pstep_option_decl *option);

extern void pipeline_decl_add_port(pipeline_decl *decl,
                                   /*@keep@*/ port_declaration *port);

extern void pipeline_decl_add_step(pipeline_decl *decl,
                                   /*@keep@*/ pipeline_step *step)
  /*@modifies decl @*/;

extern void pipeline_decl_import(pipeline_decl *decl,
                                 const xmlChar *uri)
  /*@modifies decl->imports @*/;

extern void pipeline_branch_add_step(pipeline_decl *decl,
                                     pipeline_branch *branch,
                                     /*@keep@*/ pipeline_step *step)
  /*@modifies decl->names,branch @*/;

extern void pipeline_step_add_step(pipeline_decl *decl,
                                   pipeline_step *step,
                                   /*@keep@*/ pipeline_step *innerstep)
  /*@modifies decl->names,step @*/;

extern void pipeline_step_add_option(pipeline_step *step,
                                     /*@keep@*/ pipeline_option *option);

extern void pipeline_step_add_input_port(pipeline_step *step,
                                         /*@keep@*/ input_port_instance *inp);

extern void pipeline_step_add_output_port(pipeline_step *step,
                                          /*@keep@*/ output_port_instance *outp);

extern void pipeline_step_add_assignment(pipeline_step *step,
                                         /*@keep@*/ pipeline_assignment *assign);
extern void pipeline_step_add_branch(pipeline_step *step,
                                     /*@keep@*/ pipeline_branch *branch);



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PIPELINE_H */
