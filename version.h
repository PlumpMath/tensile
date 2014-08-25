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
 * @brief implementation version info
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef XPROC_VERSION_H
#define XRPC_VERSION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#define XPROC_PRODUCT_NAME "Tensile"
#define XPROC_PRODUCT_VERSION "0.1"
#define XPROC_VENDOR "Artem V. Andreev"
#define XPROC_VENDOR_URI "https://github.com/aa5779/tensile"
#define XPROC_VERSION_AS_STRING(_version) (#_version)
#define XPROC_SUPPORTED_VERSION 1.0
#define XPROC_SUPPORTED_XPATH_VERSION 1.0
#define XPROC_SUPPORTED_VERSION_S \
  XPROC_VERSION_AS_STRING(XPROC_SUPPORTED_VERSION)
#define XPROC_SUPPORTED_XPATH_VERSION_S \
  XPROC_VERSION_AS_STRING(XPROC_SUPPORTED_XPATH_VERSION)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XPROC_VERSION_H */
