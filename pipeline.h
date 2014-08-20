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

enum port_source_kind {
  PORT_SOURCE_INLINE,
  PORT_SOURCE_DOCUMENT,
  PORT_SOURCE_DATA,
  PORT_SOURCE_PIPE,
  PORT_SOURCE_UNRESOLVED
} port_source_kind;

typedef struct output_port_instance {
  struct pipeline_step *owner;
  struct port_declaration *decl;
  xmlListPtr /* xmlDocPtr */ queue;
  struct input_port_instance *connected;
} output_port_instance;

typedef struct port_source {
  enum port_source_kind kind;
  union {
    struct {
      const char *uri;
      const char *wrapper;
      const char *wrapper_ns;
      const char *content_type;
    } load;
    xmlDocPtr doc;
    output_port_instance *pipe;
    struct {
      const char *step;
      const char *port;
    } ref;
  } x;
} port_source;

typedef struct port_connection {
  xmlXPathCompExprPtr filter;
  xmlListPtr /* port_source */ sources; 
} port_connection;

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

typedef struct input_port_instance {
  struct pipeline_step *owner;
  struct port_declaration *decl;
  port_connection *connection;
} input_port_instance;

typedef struct pstep_option_decl {
  const char *name;
  xmlXPathCompExprPtr defval;
} pstep_option_decl;

typedef struct pipeline_decl {
  const char *ns;
  const char *name;
  xmlListPtr /* pstep_option_decl */ options;
  xmlListPtr /* port_declaration */ ports;
  struct pipeline_step *body;
} pipeline_decl;

typedef struct pipeline_step_type {
  pipeline_decl decl;
  pstep_instantiate_func instantiate;
  pstep_destroy_func destroy;
  pstep_execute_func execute;
} pipeline_step_type;

typedef struct pipeline_option {
  pstep_option_decl *decl;
  port_connection value;
} pipeline_option;

enum pipeline_step_kind {
  PSTEP_END,
  PSTEP_ATOMIC,
  PSTEP_CALL,
  PSTEP_FOREACH,
  PSTEP_VIEWPORT,
  PSTEP_CHOOSE,
  PSTEP_GROUP,
  PSTEP_TRY
};

typedef struct pipeline_step {
  enum pipeline_step_kind kind;
  xmlListPtr /* input_port_instance */ inputs;
  xmlListPtr /* output_port_instance */ outputs;
  xmlListPtr /* pipeline_option */ options;
  union {
    pipeline_step_type *atomic;
    pipeline_decl *call;
    struct pipeline_step *group;
    j
  } x;
} pipeline_step;


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PIPELINE_H */
