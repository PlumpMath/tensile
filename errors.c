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

#include <libxml/list.h>
#include <setjmp.h>
#include "pipeline.h"

typedef struct pipeline_cleanup {
  pipeline_cleanup_func func;
  /*@owned@*/ void *data;
} pipeline_cleanup;

typedef struct pipeline_error_context {
  sigjmp_buf location;
  /*@owned@*/ xmlListPtr cleanups;
  /*@only@*/ pipeline_error_info *info;
  struct pipeline_error_context *chain;
} pipeline_error_context;

static pipeline_error_context *current_context;

void pipeline_run_with_error_handler(pipeline_guarded_func body,
                                     /*@null@*/ void *data,
                                     pipeline_error_handler handler,
                                     /*@null@*/ void *handler_data) {
  pipeline_error_context context;
  context.cleanups = xmlListCreate(pipeline_cleanup_destroy, NULL);
  context.info = NULL;
  context.chain = current_context;
  current_context = &context;
  if (sigsetjmp(context.location, 1) == 0) {
    body(data);

    assert(context.info == NULL);
    current_context = context.chain;
    if (current_context == NULL) {
      unwind_cleanups(context.cleanups);
    } else {
      xmlListMerge(current_context->cleanups, context.cleanups);
    }
    return;
  }
  current_context = context.chain;
  unwind_cleanups(context.cleanups);
  if (handler != NULL) {
    handler(handler_data, context.info);
  }
  pipeline_escalate_error(context.info);
}
