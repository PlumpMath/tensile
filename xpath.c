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

#include <stdbool.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

static bool split_qname(xmlXPathContextPtr ctx,
                        char *qname,
                        const char **uri,
                        const char **local) {
  char *sep = strchr(qname, ':');
  if (sep == NULL) {
    *uri = NULL;
    *local = qname;
    return true;
  } else {
    *sep = '\0';
    *uri = xmlXPathNsLookup(ctx, qname);
    *local = sep + 1;
    return *uri != NULL;
  }
}

static void pipeline_xpath_system_property(xmlXPathParserContextPtr ctxt, 
                                           int nargs) {
  if (nargs != 1) {
    xmlXPathSetArityError(ctxt);
    return;
  }
}
