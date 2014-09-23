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
#include <string.h>
#include <inttypes.h>
#include <libxml/list.h>
#include <libxml/xpathInternals.h>
#include <setjmp.h>
#include "pipeline.h"

typedef struct pipeline_cleanup {
  pipeline_cleanup_func func;
  /*@owned@*/ /*@null@*/ void *data;
  /*@dependent@*/ /*@null@*/ const void *frame;
} pipeline_cleanup;

typedef struct pipeline_error_context {
  /*@dependent@*/ sigjmp_buf location;
  /*@owned@*/ xmlListPtr cleanups;
  /*@only@*/ pipeline_error_info *info;
  /*@dependent@*/ volatile struct pipeline_error_context *chain;
} pipeline_error_context;

static /*@dependent@*/ /*@null@*/ volatile pipeline_error_context *current_context;

static void unwind_cleanups(/*@only@*/ xmlListPtr cleanups) {
  for (;;) {
    xmlLinkPtr head = xmlListFront(cleanups);
    pipeline_cleanup *cleanup;
    
    if (head == NULL)
      break;
    cleanup = xmlLinkGetData(head);
    if (cleanup != NULL) {
      assert(cleanup->func != NULL || cleanup->data == NULL);
      cleanup->func(cleanup->data);
    }
    xmlListPopFront(cleanups);
    xmlFree(cleanup);
  }
  xmlListDelete(cleanups);
}

void pipeline_error_info_destroy(pipeline_error_info *err) {
  if (err == NULL)
    return;
  
  xmlFree(err->step_name);
  xmlFree(err->step_type);
  xmlFree(err->step_type_ns);
  xmlFree(err->code);
  xmlFree(err->code_ns);
  xmlFree(err->href);
  xmlFree(err->details);
  xmlFree(err);
}

static int compare_cleanup_data(const void *data0, const void *data1) {
  return (int)((intptr_t)(((const pipeline_cleanup *)data1)->data) -
               (intptr_t)(((const pipeline_cleanup *)data0)->data));
}  

void pipeline_run_with_error_handler(pipeline_guarded_func body,
                                     /*@null@*/ void *data,
                                     pipeline_error_handler handler,
                                     /*@null@*/ void *handler_data) {
  volatile pipeline_error_context context;

  context.cleanups = xmlListCreate(NULL, compare_cleanup_data);
  context.info = NULL;
  context.chain = current_context;
  current_context = &context;
  if (sigsetjmp(((pipeline_error_context *)&context)->location, 1) == 0) {
    body(data);

    assert(context.info == NULL);
    assert(current_context == &context);
    current_context = context.chain;
    if (current_context == NULL) {
      unwind_cleanups(context.cleanups);
    } else {
      xmlListMerge(current_context->cleanups, context.cleanups);
      xmlListDelete(context.cleanups);
    }
    return;
  }
  assert(context.info != NULL);
  current_context = context.chain;
  unwind_cleanups(context.cleanups);
  if (handler != NULL) {
    handler(handler_data, context.info);
  }
  pipeline_error_info_destroy(context.info);
}

static void print_error_info(const pipeline_error_info *err) {
  fprintf(stderr, "Error %s%s%s at step %s of type %s%s%s in %s:%d:%d (@ %d): %s\n",
          err->code_ns != NULL ? (char *)err->code_ns : "",
          err->code_ns != NULL ? ":" : "",
          err->code != NULL ? (char *)err->code : "unknown",
          err->step_name != NULL ? (char *)err->step_name : "unknown",
          err->step_type_ns != NULL ? (char *)err->step_type_ns : "",
          err->step_type_ns != NULL ? ":" : "",
          err->step_type != NULL ? (char *)err->step_type : "unknown",
          err->href != NULL ? (char *)err->href : "???",
          err->lineno, err->columno, err->offset,
          err->details != NULL ? (char *)err->details : "");
}


void pipeline_escalate_error(pipeline_error_info *err) {
  if (current_context == NULL) {
    print_error_info(err);
    exit(EXIT_FAILURE);
  }
  assert(current_context->info == NULL);
  current_context->info = err;
  siglongjmp(((pipeline_error_context *)current_context)->location, 1);
}

  
extern void pipeline_report_error(const xmlChar *step_name,
                                  const xmlChar *step_type,
                                  const xmlChar *step_type_ns,
                                  const xmlChar *code,
                                  const xmlChar *code_ns,
                                  const xmlChar *href,
                                  int lineno,
                                  int columno,
                                  int offset,
                                  const xmlChar *details) {
  pipeline_error_info *err = xmlMalloc(sizeof(*err));
  err->step_name = xmlStrdup(step_name);
  err->step_type = xmlStrdup(step_type);
  err->step_type_ns = xmlStrdup(step_type_ns);
  err->code = xmlStrdup(code);
  err->code_ns = xmlStrdup(code_ns);
  err->href = xmlStrdup(href);
  err->lineno = lineno;
  err->columno = columno;
  err->offset = offset;
  err->details = xmlStrdup(details);
  pipeline_escalate_error(err);
}

void pipeline_report_os_error(int err) {
  xmlChar code[8];
  xmlStrPrintf(code, (int)sizeof(code),
               (const xmlChar *)"OS%4.4d", err);
  pipeline_report_error(NULL, NULL, NULL,
                        code, NULL,
                        NULL, 0, 0, 0,
                        (const xmlChar *)strerror(err));
}


void pipeline_report_xml_error(xmlErrorPtr err) {
  if (err == NULL) {
    pipeline_report_error(NULL, NULL, NULL,
                          (const xmlChar *)"XML00-0000", NULL,
                          NULL, 0, 0, 0, 
                          (const xmlChar *)"No error");
  } else {
    xmlChar code[16];
    xmlStrPrintf(code, (int)sizeof(code), 
                 (const xmlChar *)"XML%2.2d-%4.4d-%d", 
                 err->domain, err->code);
    pipeline_report_error(NULL, NULL, NULL, 
                          code, NULL,
                          (const xmlChar *)err->file,
                          err->line, err->int2 /* column */, 0,
                          (const xmlChar *)err->message);
  }
}

static /*@observer@*/ const char * const static_errors_desc[] = {
  [XPROC_ERR_STATIC_CONNECTION_LOOP - XPROC_ERR_STATIC_FIRST] =
  "There are any loops in the connections between steps: no step can be"
  " connected to itself nor can there be any sequence of connections through"
  " other steps that leads back to itself",
  [XPROC_ERR_STATIC_DUPLICATE_STEP_NAME - XPROC_ERR_STATIC_FIRST] = 
  "All steps in the same scope must have unique names: it is a static error if"
  " two steps with the same name appear in the same scope",
  [XPROC_ERR_STATIC_UNCONNECTED_INPUT - XPROC_ERR_STATIC_FIRST] = 
  "A declared input is not connected",
  [XPROC_ERR_STATIC_DUPLICATE_BINDING - XPROC_ERR_STATIC_FIRST] = 
  "A option or variable declaration duplicates the name of any other option or"
  " variable in the same environment",
  [XPROC_ERR_STATIC_UNCONNECTED_PRIMARY_OUTPUT - XPROC_ERR_STATIC_FIRST] = 
  "The primary output port of a step is not connected",
  [XPROC_ERR_STATIC_NO_PRIMARY_OUTPUT - XPROC_ERR_STATIC_FIRST] = 
  "The primary output port has no explicit connection and the last step in "
  " the subpipeline does not have a primary output port",
  [XPROC_ERR_STATIC_INCONSISTENT_OUTPUTS - XPROC_ERR_STATIC_FIRST] = 
  "Two subpipelines in a p:choose declare different outputs",
  [XPROC_ERR_STATIC_INVALID_ATTRIBUTE - XPROC_ERR_STATIC_FIRST] = 
  "An element in the XProc namespace has attributes not defined by this "
  " specification unless they are extension attributes",
  [XPROC_ERR_STATIC_INCONSISTENT_CATCH_OUTPUTS - XPROC_ERR_STATIC_FIRST] = 
  "The p:group and p:catch subpipelines declare different outputs.",
  [XPROC_ERR_STATIC_NONCONFORMING_STEP - XPROC_ERR_STATIC_FIRST] = 
  "A pipeline contains a step whose specified inputs, outputs, and options "
  " do not match the signature for steps of that type",
  [XPROC_ERR_STATIC_DUPLICATE_PORT_NAME - XPROC_ERR_STATIC_FIRST] = 
  "Two ports on the same step have the same name",
  [XPROC_ERR_STATIC_DUPLICATE_PRIMARY_OUTPUT - XPROC_ERR_STATIC_FIRST] = 
  "More than one output port is identified as primary.",
  [XPROC_ERR_STATIC_EMPTY_COMPOUND - XPROC_ERR_STATIC_FIRST] = 
  "A compound step has no contained steps",
  [XPROC_ERR_STATIC_INCONSISTENT_OPTION_DECL - XPROC_ERR_STATIC_FIRST] = 
  "An option is both required and has a default value.",
  [XPROC_ERR_STATIC_MISSING_REQUIRED_OPTION - XPROC_ERR_STATIC_FIRST] = 
  "A step is invoked without specifying a value for a required option",
  [XPROC_ERR_STATIC_INVALID_VARIABLE_CONNECTION - XPROC_ERR_STATIC_FIRST] = 
  "A variable\'s document connection refers to the output port of a step "
  " in the surrounding container's contained steps",
  [XPROC_ERR_STATIC_INVALID_NAMESPACE - XPROC_ERR_STATIC_FIRST] = 
  "The binding attribute on p:namespaces is specified and its value is not"
  " the name of an in-scope binding",
  [XPROC_ERR_STATIC_INVALID_PIPE - XPROC_ERR_STATIC_FIRST] = 
  "The port identified by a p:pipe is not in the readable ports of"
  " the step that contains the p:pipe",
  [XPROC_ERR_STATIC_MALFORMED_INLINE - XPROC_ERR_STATIC_FIRST] = 
  "The content of the p:inline element does not consist of exactly one element,"
  " optionally preceded and/or followed by any number of processing instructions,"
  " comments or whitespace characters",
  [XPROC_ERR_STATIC_INVALID_TYPE_NS - XPROC_ERR_STATIC_FIRST] = 
  "The expanded-QName value of the type attribute is in no namespace or in the XProc namespace",
  [XPROC_ERR_STATIC_INVALID_LOG - XPROC_ERR_STATIC_FIRST] = 
  "The port specified on the p:log is not the name of an output port"
  " on the step in which it appears or more than one p:log element"
  " is applied to the same port",
  [XPROC_ERR_STATIC_INCONSISTENT_OPTION - XPROC_ERR_STATIC_FIRST] = 
  "An option is specified with both the shortcut form and the long form",
  [XPROC_ERR_STATIC_RESERVED_NS - XPROC_ERR_STATIC_FIRST] = 
  "An option or variable is declared in the XProc namespace.",
  [XPROC_ERR_STATIC_CONNECTED_OUTPUT_IN_ATOMIC_STEP - XPROC_ERR_STATIC_FIRST] = 
  "A connection for a p:output is specified inside a p:declare-step for an atomic step",
  [XPROC_ERR_STATIC_DUPLICATE_PRIMARY_INPUT - XPROC_ERR_STATIC_FIRST] = 
  "More than one input port is specified as the primary",
  [XPROC_ERR_STATIC_UNDECLARED_OPTION - XPROC_ERR_STATIC_FIRST] = 
  "An option is on an atomic step used that is not declared on steps of that type",
  [XPROC_ERR_STATIC_NO_DEFAULT_INPUT_PORT - XPROC_ERR_STATIC_FIRST] = 
  "No connection is provided and the default readable port is undefined",
  [XPROC_ERR_STATIC_INVALID_PORT_KIND - XPROC_ERR_STATIC_FIRST] = 
  "The kind of input is neither 'document' nor 'parameter'",
  [XPROC_ERR_STATIC_INVALID_PARAMETER_PORT - XPROC_ERR_STATIC_FIRST] = 
  "The specified port is not a parameter input port or no port is specified and" 
  " the step does not have a primary parameter input port",
  [XPROC_ERR_STATIC_CONNECTED_PARAMETER_PORT - XPROC_ERR_STATIC_FIRST] = 
  "The declaration of a parameter input port contains a connection;"
  " parameter input port declarations must be empty",
  [XPROC_ERR_STATIC_DUPLICATE_TYPE - XPROC_ERR_STATIC_FIRST] = 
  "The step types in a pipeline or library must have unique names:"
  " it is a static error if any step type name is built-in and/or"
  " declared or defined more than once in the same scope",
  [XPROC_ERR_STATIC_MISPLACED_TEXT_NODE - XPROC_ERR_STATIC_FIRST] = 
  "A step directly contains text nodes that do not consist entirely of whitespace",
  [XPROC_ERR_STATIC_MISSING_REQUIRED_ATTRIBUTE - XPROC_ERR_STATIC_FIRST] = 
  "A required attribute is not provided.",
  [XPROC_ERR_STATIC_INVALID_SERIALIZATION - XPROC_ERR_STATIC_FIRST] = 
  "The port specified on the p:serialization is not the name of an output port"
  " on the pipeline in which it appears or if more than one p:serialization"
  " element is applied to the same port",
  [XPROC_ERR_STATIC_PARAMETER_PORT_NOT_SEQUENCE - XPROC_ERR_STATIC_FIRST] = 
  "Parameter ports must be sequence ports",
  [XPROC_ERR_STATIC_INCONSISTENT_NAMESPACE_BINDING - XPROC_ERR_STATIC_FIRST] = 
  "Both binding and element are specified on the same p:namespaces element",
  [XPROC_ERR_STATIC_CONNECTED_INPUT_IN_ATOMIC_STEP - XPROC_ERR_STATIC_FIRST] = 
  "A connection for an input port is provided on the declaration of an atomic step",
  [XPROC_ERR_STATIC_UNDECLARED_STEP - XPROC_ERR_STATIC_FIRST] = 
  "An element in the XProc namespace or a step has element children other than"
  " those specified for it by this specification",
  [XPROC_ERR_STATIC_DECLARED_STEP_IS_COMPOUND - XPROC_ERR_STATIC_FIRST] = 
  "A declared step is used as a compound step",
  [XPROC_ERR_STATIC_INVALID_EXCEPT_PREFIXES - XPROC_ERR_STATIC_FIRST] = 
  "The except-prefixes attribute on p:namespaces does not contain a list of tokens or"
  " any of those tokens is not a prefix bound to a namespace in the in-scope namespaces"
  " of the p:namespaces element",
  [XPROC_ERR_STATIC_CANNOT_IMPORT - XPROC_ERR_STATIC_FIRST] = 
  "The URI of a p:import cannot be retrieved or it does not point to a p:library,"
  " p:declare-step, or p:pipeline",
  [XPROC_ERR_STATIC_IMPORT_WITHOUT_TYPE - XPROC_ERR_STATIC_FIRST] = 
  "A single pipeline is imported that does not have a type",
  [XPROC_ERR_STATIC_UNCONNECTED_PARAMETER_INPUT - XPROC_ERR_STATIC_FIRST] = 
  "A primary parameter input port is unconnected and the pipeline that contains"
  " the step has no primary parameter input port unless at least one explicit"
  " p:with-param is provided for that port",
  [XPROC_ERR_STATIC_INVALID_EXCLUDE_INLINE_PREFIXES - XPROC_ERR_STATIC_FIRST] = 
  "The exclude-inline-prefixes attribute does not contain a list of tokens or"
  " any of those tokens (except #all or #default) is not a prefix bound to"
  " a namespace in the in-scope namespaces of the element on which it occurs",
  [XPROC_ERR_STATIC_NO_DEFAULT_FOR_EXCLUDE_INLINE_PREFIXES - XPROC_ERR_STATIC_FIRST] = 
  "The value #default is used within the exclude-inline-prefixes attribute"
  " and there is no default namespace in scope.",
  [XPROC_ERR_STATIC_INVALID_TOPLEVEL_ELEMENT - XPROC_ERR_STATIC_FIRST] = 
  "The pipeline element is not p:pipeline, p:declare-step, or p:library",
  [XPROC_ERR_STATIC_UNSUPPORTED_VERSION - XPROC_ERR_STATIC_FIRST] = 
  "The processor encounters an explicit request for a previous version"
  " of the language and it is unable to process the pipeline using those semantics",
  [XPROC_ERR_STATIC_NONSTATIC_USE_WHEN - XPROC_ERR_STATIC_FIRST] = 
  "A use-when expression refers to the context or attempts to refer to any documents or collections",
  [XPROC_ERR_STATIC_UNDEFINED_VERSION - XPROC_ERR_STATIC_FIRST] = 
  "A required version attribute is not present",
  [XPROC_ERR_STATIC_MALFORMED_VERSION - XPROC_ERR_STATIC_FIRST] = 
  "The value of the version attribute is not a xs:decimal"
};

static const char * const dynamic_errors_desc[] = {
  [XPROC_ERR_DYNAMIC_NONXML_OUTPUT - XPROC_ERR_DYNAMIC_FIRST] =
  "A non-XML resource is produced on a step output or arrives on a step input",
  [XPROC_ERR_DYNAMIC_SEQUENCE_IN_VIEWPORT_SOURCE - XPROC_ERR_STATIC_FIRST] = 
  "The viewport source does not provide exactly one document",
  [XPROC_ERR_DYNAMIC_NOTHING_IS_CHOSEN - XPROC_ERR_STATIC_FIRST] = 
  "No subpipeline is selected by the p:choose and no default is provided",
  [XPROC_ERR_DYNAMIC_SEQUENCE_IN_XPATH_CONTEXT - XPROC_ERR_STATIC_FIRST] = 
  "More than one document appears on the connection for the xpath-context",
  [XPROC_ERR_DYNAMIC_UNEXPECTED_INPUT_SEQUENCE - XPROC_ERR_STATIC_FIRST] = 
  "More than one or no document appears on the declared port which is not a sequence port",
  [XPROC_ERR_DYNAMIC_UNEXPECTED_OUTPUT_SEQUENCE - XPROC_ERR_STATIC_FIRST] = 
  "The step does not produce exactly one document on the declared port"
  " which is not a sequence port", 
  [XPROC_ERR_DYNAMIC_SEQUENCE_AS_CONTEXT_NODE - XPROC_ERR_STATIC_FIRST] = 
  "A document sequence appears where a document to be used as the context node is expected",
  [XPROC_ERR_DYNAMIC_INVALID_NAMESPACE_REF - XPROC_ERR_STATIC_FIRST] = 
  "The element attribute on p:namespaces is specified and it does not"
  " identify a single element node",
  [XPROC_ERR_DYNAMIC_NO_VIEWPORT_MATCH - XPROC_ERR_STATIC_FIRST] = 
  "The match expression on p:viewport does not match an element or document",
  [XPROC_ERR_DYNAMIC_INVALID_DOCUMENT - XPROC_ERR_STATIC_FIRST] = 
  "The resource referenced by a p:document element does not exist,"
  " cannot be accessed, or is not a well-formed XML document",
  [XPROC_ERR_DYNAMIC_UNSUPPORTED_URI_SCHEME - XPROC_ERR_STATIC_FIRST] = 
  "An attempt is made to dereference a URI where the scheme of the URI reference is not supported",
  [XPROC_ERR_DYNAMIC_INCONSISTENT_NAMESPACE_BINDING - XPROC_ERR_STATIC_FIRST] = 
  "The same prefix is bound to two different namespace names",
  [XPROC_ERR_DYNAMIC_MALFORMED_PARAM - XPROC_ERR_STATIC_FIRST] = 
  "A unqualified attribute name other than \"name\", \"namespace\", or \"value\""
  " appears on a c:param element",
  [XPROC_ERR_DYNAMIC_UNRESOLVED_QNAME - XPROC_ERR_STATIC_FIRST] = 
  "The specified QName cannot be resolved with the in-scope namespace declarations",
  [XPROC_ERR_DYNAMIC_BAD_INPUT_SELECT - XPROC_ERR_STATIC_FIRST] = 
  "The select expression on a p:input returns atomic values or anything other than"
  " element or document nodes (or an empty sequence)",
  [XPROC_ERR_DYNAMIC_UNSUPPORTED_STEP - XPROC_ERR_STATIC_FIRST] = 
  "The running pipeline attempts to invoke a step which the processor does not know how to perform",
  [XPROC_ERR_DYNAMIC_MALFORMED_PARAM_LIST - XPROC_ERR_STATIC_FIRST] = 
  "The parameter list contains some elements that are not c:param.",
  [XPROC_ERR_DYNAMIC_INVALID_OPTION_TYPE - XPROC_ERR_STATIC_FIRST] = 
  "An option value does not satisfy the type required for that option",
  [XPROC_ERR_DYNAMIC_INVALID_SERIALIZATION_OPTIONS - XPROC_ERR_STATIC_FIRST] = 
  "The combination of serialization options specified or defaulted is not allowed",
  [XPROC_ERR_DYNAMIC_ACCESS_DENIED - XPROC_ERR_STATIC_FIRST] = 
  "A pipeline attempts to access a resource for which it has insufficient privileges"
  " or perform a step which is forbidden",
  [XPROC_ERR_DYNAMIC_NEED_PSVI - XPROC_ERR_STATIC_FIRST] = 
  "A processor that does not support PSVI annotations attempts"
  " to invoke a step which asserts that they are required",
  [XPROC_ERR_DYNAMIC_BAD_XPATH - XPROC_ERR_STATIC_FIRST] = 
  "A XPath expression is encountered which cannot be evaluated"
  "(because it is syntactically incorrect, contains references to unbound variables"
  " or unknown functions, or for any other reason)",
  [XPROC_ERR_DYNAMIC_NEED_XPATH10 - XPROC_ERR_STATIC_FIRST] = 
  "A 2.0 processor encounters an XPath 1.0 expression"
  " and it does not support XPath 1.0 compatibility mode",
  [XPROC_ERR_DYNAMIC_INVALID_PARAM_NAMESPACE - XPROC_ERR_STATIC_FIRST] = 
  "The namespace attribute is specified, the name contains a colon,"
  "and the specified namespace is not the same as the in-scope namespace binding for the specified prefix",
  [XPROC_ERR_DYNAMIC_NO_CONTEXT - XPROC_ERR_STATIC_FIRST] = 
  "The select expression makes reference to the context node, size,"
  "or position when the context item is undefined",
  [XPROC_ERR_DYNAMIC_UNSUPPORTED_XPATH_VERSION - XPROC_ERR_STATIC_FIRST] = 
  "The processor encounters an xpath-version that it does not support",
  [XPROC_ERR_DYNAMIC_INVALID_ATTRIBUTE_TYPE - XPROC_ERR_STATIC_FIRST] = 
  "An attribute value does not satisfy the type required for that attribute",
  [XPROC_ERR_DYNAMIC_BAD_DATA - XPROC_ERR_STATIC_FIRST] = 
  "The document referenced by a p:data element does not exist,"
  " cannot be accessed, or cannot be encoded as specified",
  [XPROC_ERR_DYNAMIC_STEP_FAILED - XPROC_ERR_STATIC_FIRST] = 
  "A step is unable or incapable of performing its function",
  [XPROC_ERR_DYNAMIC_RESERVED_NAMESPACE - XPROC_ERR_STATIC_FIRST] = 
  "The XProc namespace is used in the name of a parameter",
  [XPROC_ERR_DYNAMIC_VALUE_NOT_AVAILABLE - XPROC_ERR_STATIC_FIRST] = 
  "The name specified is not the name of an in-scope option or variable",
  [XPROC_ERR_DYNAMIC_INCONSISTENT_NAMESPACE - XPROC_ERR_STATIC_FIRST] = 
  "A new namespace or prefix is specified but the lexical value of"
  " the specified name contains a colon and no wrapper is explicitly specified",
};

static const char * const step_errors_desc[] = {
  [XPROC_ERR_STEP_MALFORMED_MIME_BOUNDARY - XPROC_ERR_STEP_FIRST] = 
  "The value starts with the string \"--\"",
  [XPROC_ERR_STEP_NO_AUTH_METHOD - XPROC_ERR_STEP_FIRST] = 
  "A username or password is specified without specifying an auth-method,"
  " or the requested auth-method isn't supported,"
  " or the authentication challenge contains an authentication method"
  " that isn't supported",
  [XPROC_ERR_STEP_INVALID_STATUS_ONLY_FLAG - XPROC_ERR_STEP_FIRST] = 
  "The status-only attribute has the value true and"
  " the detailed attribute does not have the value true",
  [XPROC_ERR_STEP_ENTITY_BODY_NOT_ALLOWED - XPROC_ERR_STEP_FIRST] = 
  "The request contains a c:body or c:multipart but the method "
  "does not allow for an entity body being sent with the request",
  [XPROC_ERR_STEP_NO_METHOD - XPROC_ERR_STEP_FIRST] = 
  "The method is not specified on a c:request",
  [XPROC_ERR_STEP_INVALID_CHARSET - XPROC_ERR_STEP_FIRST] = 
  "An encoding of base64 is specified and the character set is not specified"
  " or the specified character set is not supported by the implementation",
  [XPROC_ERR_STEP_DIRECTORY_ACCESS_DENIED - XPROC_ERR_STEP_FIRST] = 
  "The contents of the directory path are not available to the step"
  " due to access restrictions in the environment in which the pipeline is run",
  [XPROC_ERR_STEP_INCONSISTENT_RENAME - XPROC_ERR_STEP_FIRST] = 
  "The pattern matches a processing instruction and the new name has a non-null namespace",
  [XPROC_ERR_STEP_RENAME_RESERVED_NS - XPROC_ERR_STEP_FIRST] = 
  "The XML namespace or the XMLNS namespace is the value of either the from option or the to option",
  [XPROC_ERR_STEP_NOT_A_DIRECTORY - XPROC_ERR_STEP_FIRST] = 
  "The absolute path does not identify a directory",
  [XPROC_ERR_STEP_DOCUMENTS_NOT_EQUAL - XPROC_ERR_STEP_FIRST] = 
  "The documents are not equal, and the value of the fail-if-not-equal option is true",
  [XPROC_ERR_STEP_INCONSISTENT_VALUES - XPROC_ERR_STEP_FIRST] = 
  "The user specifies a value or values that are inconsistent"
  " with each other or with the requirements of the step or protocol",
  [XPROC_ERR_STEP_MALFORMED_BODY - XPROC_ERR_STEP_FIRST] = 
  "The content of the c:body element does not consist of exactly one element,"
  " optionally preceded and/or followed by any number of processing instructions,"
  " comments or whitespace characters",
  [XPROC_ERR_STEP_INVALID_NODE_TYPE - XPROC_ERR_STEP_FIRST] = 
  "A select expression or match pattern returns a node type that is not allowed by the step",
  [XPROC_ERR_STEP_INVALID_INSERT - XPROC_ERR_STEP_FIRST] = 
  "The match pattern matches anything other than an element node and"
  " the value of the position option is \"first-child\" or \"last-child\"",
  [XPROC_ERR_STEP_DTD_CANNOT_VALIDATE - XPROC_ERR_STEP_FIRST] = 
  "The document is not valid or the step doesn't support DTD validation",
  [XPROC_ERR_STEP_TAGS_IN_BODY - XPROC_ERR_STEP_FIRST] = 
  "The content of the c:body element does not consist entirely of characters",
  [XPROC_ERR_STEP_XINCLUDE_ERROR - XPROC_ERR_STEP_FIRST] = 
  "An XInclude error occurs during processing",
  [XPROC_ERR_STEP_INVALID_OVERRIDE_CONTENT_TYPE - XPROC_ERR_STEP_FIRST] = 
  "The override-content-type value cannot be used (e.g. text/plain to override image/png)",
  [XPROC_ERR_STEP_COMMAND_CANNOT_RUN - XPROC_ERR_STEP_FIRST] = 
  "The command cannot be run",
  [XPROC_ERR_STEP_CANNOT_CHANGE_DIRECTORY - XPROC_ERR_STEP_FIRST] =
  "The current working directory cannot be changed to the value of the cwd option",
  [XPROC_ERR_STEP_INCONSISTENT_EXEC_WRAPPER - XPROC_ERR_STEP_FIRST] = 
  "Both result-is-xml and wrap-result-lines are specified",
  [XPROC_ERR_STEP_UNSUPPORTED_HASH_ALGO - XPROC_ERR_STEP_FIRST] = 
  "The requested hash algorithm is not one that the processor understands"
  " or the value or parameters are not appropriate for that algorithm",
  [XPROC_ERR_STEP_NOT_URLENCODED - XPROC_ERR_STEP_FIRST] = 
  "The value provided is not a properly x-www-form-urlencoded value",
  [XPROC_ERR_STEP_UNSUPPORTED_XSLT_VERSION - XPROC_ERR_STEP_FIRST] = 
  "The specified version of XSLT is not available",
  [XPROC_ERR_STEP_SEQUENCE_FOR_XSLT - XPROC_ERR_STEP_FIRST] = 
  "A sequence of documents (including an empty sequence) is provided to an XSLT 1.0 step",
  [XPROC_ERR_STEP_NOT_A_REQUEST - XPROC_ERR_STEP_FIRST] = 
  "The document element of the document that arrives on the source port is not c:request",
  [XPROC_ERR_STEP_CANNOT_STORE - XPROC_ERR_STEP_FIRST] = 
  "The URI scheme is not supported or the step cannot store to the specified location",
  [XPROC_ERR_STEP_UNSUPPORTED_CONTENT_TYPE - XPROC_ERR_STEP_FIRST] = 
  "The content-type specified is not supported by the implementation",
  [XPROC_ERR_STEP_UNSUPPORTED_ENCODING - XPROC_ERR_STEP_FIRST] = 
  "The encoding specified is not supported by the implementation",
  [XPROC_ERR_STEP_DOCUMENT_NOT_VALID - XPROC_ERR_STEP_FIRST] = 
  "The assert-valid option is true and the input document is not valid",
  [XPROC_ERR_STEP_SCHEMATRON_ASSERTION_FAIL - XPROC_ERR_STEP_FIRST] = 
  "The assert-valid option is true and any Schematron assertions fail",
  [XPROC_ERR_STEP_UNSUPPORTED_VALIDATION_MODE - XPROC_ERR_STEP_FIRST] = 
  "The implementation does not support the specified validation mode",
  [XPROC_ERR_STEP_CANNOT_APPLY_TEMPLATE - XPROC_ERR_STEP_FIRST] = 
  "The specified initial mode or named template cannot be applied to the specified stylesheet",
  [XPROC_ERR_STEP_MALFORMED_XQUERY_RESULT - XPROC_ERR_STEP_FIRST] = 
  "The sequence that results from evaluating the XQuery contains items other than documents and elements",
  [XPROC_ERR_STEP_INVALID_ADD_BASE - XPROC_ERR_STEP_FIRST] =
  "The all and relative options are both true",
  [XPROC_ERR_STEP_INVALID_ADD_ATTRIBUTE - XPROC_ERR_STEP_FIRST] = 
  "The QName value in the attribute-name option uses the prefix \"xmlns\""
  " or any other prefix that resolves to the namespace name \"http://www.w3.org/2000/xmlns/\"",
  [XPROC_ERR_STEP_UNSUPPORTED_UUID_VERSION - XPROC_ERR_STEP_FIRST] = 
  "The processor does not support the specified version of the UUID algorithm",
  [XPROC_ERR_STEP_INVALID_NCNAME - XPROC_ERR_STEP_FIRST] = 
  "The name of any encoded parameter name is not a valid xs:NCName",
  [XPROC_ERR_STEP_DELETE_NAMESPACE_NODE - XPROC_ERR_STEP_FIRST] = 
  "The match option for p:delete matches a namespace node",
  [XPROC_ERR_STEP_INVALID_PATH_SEPARATOR - XPROC_ERR_STEP_FIRST] =
  "The path-separator option is specified and is not exactly one character long",
  [XPROC_ERR_STEP_COMMAND_EXIT_FAILURE - XPROC_ERR_STEP_FIRST] = 
  "The exit code from the command is greater than the specified failure-threshold value",
  [XPROC_ERR_STEP_INVALID_ARG_SEPARATOR - XPROC_ERR_STEP_FIRST] =
  "The arg-separator option is specified and is not exactly one character long"
};

void pipeline_report_xproc_error(const xmlChar *step_name,
                                 const xmlChar *step_type,
                                 const xmlChar *step_type_ns,
                                 xproc_error code,
                                 const xmlChar *href,
                                 int lineno,
                                 int columno,
                                 int offset) {
  xmlChar xproc_code[8];
  bool is_step_error = (code >= XPROC_ERR_STEP_FIRST);
  bool is_dynamic_error = !is_step_error && 
    (code >= XPROC_ERR_DYNAMIC_FIRST);

  xmlStrPrintf(xproc_code, (int)sizeof(xproc_code), 
               (const xmlChar *)"X%c%4.4d",
               is_step_error ? 'C' : is_dynamic_error ? 'D' : 'S',
               code - (is_step_error ? XPROC_ERR_STEP_FIRST :
                       is_dynamic_error ? XPROC_ERR_DYNAMIC_FIRST :
                       XPROC_ERR_STATIC_FIRST) + (enum xproc_error)1);
  /*@+enumindex@*/
  pipeline_report_error(step_name,
                        step_type,
                        step_type_ns,
                        xproc_code,
                        (const xmlChar *)W3C_XPROC_ERROR_NAMESPACE,
                        href, lineno, columno, offset,
                        (const xmlChar *)
                        (is_step_error ? 
                         step_errors_desc[code - XPROC_ERR_STEP_FIRST] :
                         is_dynamic_error ? 
                         dynamic_errors_desc[code - XPROC_ERR_DYNAMIC_FIRST] :
                         static_errors_desc[code - XPROC_ERR_STATIC_FIRST]));
  /*@=enumindex@*/
}

void pipeline_report_xpath_error(xmlXPathParserContextPtr ctx) {
  static const char * const xpath_errs[] = {
    [XPATH_EXPRESSION_OK] = "No error",
    [XPATH_NUMBER_ERROR] = "Invalid number",
    [XPATH_UNFINISHED_LITERAL_ERROR] = "Unfinished literal",
    [XPATH_START_LITERAL_ERROR] = "Bad literal",
    [XPATH_VARIABLE_REF_ERROR] = "Invalid variable reference",
    [XPATH_UNDEF_VARIABLE_ERROR] = "Undefined variable",
    [XPATH_INVALID_PREDICATE_ERROR] = "Invalid predicate",
    [XPATH_EXPR_ERROR] = "Bad expression",
    [XPATH_UNCLOSED_ERROR] = "Unclosed expression",
    [XPATH_UNKNOWN_FUNC_ERROR] = "Unknown function",
    [XPATH_INVALID_OPERAND] = "Invalid operand",
    [XPATH_INVALID_TYPE] = "Invalid type",
    [XPATH_INVALID_ARITY] = "Invalid arity",
    [XPATH_INVALID_CTXT_SIZE] = "Invalid context size",
    [XPATH_INVALID_CTXT_POSITION] = "Invalid context position",
    [XPATH_MEMORY_ERROR] = "Memory error",
    [XPTR_SYNTAX_ERROR] = "XPointer syntax error",
    [XPTR_RESOURCE_ERROR] = "XPointer resource error",
    [XPTR_SUB_RESOURCE_ERROR] = "XPointer subresource error",
    [XPATH_UNDEF_PREFIX_ERROR] = "Undefined namespace prefix",
    [XPATH_ENCODING_ERROR] = "Encoding error",
    [XPATH_INVALID_CHAR_ERROR] = "Invalid character",
    [XPATH_INVALID_CTXT] = "Invalid XPath context",
    [XPATH_STACK_ERROR] = "Stack error"
  };
  xmlXPathError err = xmlXPathGetError(ctx);
  /*@+enumindex@*/
  const char *description = (unsigned)err >= (unsigned)(sizeof(xpath_errs) / sizeof(*xpath_errs)) ?
    "Unknown error" :
    xpath_errs[err];
  /*@=enumindex@*/
  xmlChar code[8];
  xmlStrPrintf(code, (int)sizeof(code),
               (const xmlChar *)"XP%4.4d",
               err);
  pipeline_report_error(NULL, NULL, NULL,
                        code, NULL,
                        NULL, 0, 0, ctx->cur - ctx->base,
                        (const xmlChar *)description);
}

void pipeline_report_xproc_error_node(/*@null@*/ const xmlChar *step_name,
                                      /*@null@*/ const xmlChar *step_type,
                                      /*@null@*/ const xmlChar *step_type_ns,
                                      xproc_error code,
                                      xmlNodePtr node) {
  pipeline_report_xproc_error(step_name,
                              step_type,
                              step_type_ns,
                              code,
                              node->doc->URL,
                              (int)xmlGetLineNo(node),
                              0, 0);
}


void pipeline_add_cleanup(pipeline_cleanup_func func, 
                          void *data,
                          const void *frame) {
  pipeline_cleanup *cln;

  assert(current_context != NULL);
  if (data != NULL) {
    pipeline_cleanup *ex = xmlListSearch(current_context->cleanups, (void *)data);
    if (ex != NULL) {
      assert(ex->func == func);
      /*@-mustfreeonly@*/
      return;
      /*@=mustfreeonly@*/
    }
  }
  cln = xmlMalloc(sizeof(*cln));
  cln->func = func;
  cln->data = data;
  cln->frame = frame;
  xmlListPushFront(current_context->cleanups, cln);
}

void pipeline_remove_cleanup(const void *data, bool perform) {
  pipeline_cleanup *cln;
  if (current_context == NULL)
    return;
  cln = xmlListSearch(current_context->cleanups, (void *)data);
  if (cln == NULL)
    return;
  if (perform) {
    assert (cln->func != NULL || cln->data == NULL);
    cln->func(cln->data);
  }

  xmlListRemoveFirst(current_context->cleanups, (void *)data);
  /*@-dependenttrans@*/
  xmlFree(cln);
  /*@=dependenttrans@*/
}

void pipeline_make_cleanup_local(const void *data, const void *frame) {
  pipeline_cleanup *cln;
  if (current_context == NULL)
    return;
  cln = xmlListSearch(current_context->cleanups, (void *)data);
  if (cln == NULL)
    return;
  cln->frame = frame;
}


void pipeline_cleanup_frame(const void *frame) {
  if (current_context == NULL)
    return;

  for (;;) {
    pipeline_cleanup *cln;
    xmlLinkPtr head = xmlListFront(current_context->cleanups);
    
    if (head == NULL)
      break;
    cln = xmlLinkGetData(head);
    if (cln != NULL) {
      if (cln->frame != frame)
        break;
      
      assert(cln->data != NULL);
      cln->func(cln->data);
    }
    xmlListPopFront(current_context->cleanups);
    xmlFree(cln);
  }
}

void pipeline_run_protected(pipeline_protected_func func,
                            pipeline_cleanup_func cleanup,
                            void *data) {
  /*@-temptrans@*/
  pipeline_add_cleanup(cleanup, data, NULL);
  /*@=temptrans@*/
  func(data);
  pipeline_remove_cleanup(data, false);
}
