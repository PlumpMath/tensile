/*
 * Copyright (c) 2015  Artem V. Andreev
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
/** @file
 * @brief extern scope for generated functions
 * @remark This file may be included multiple times
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */

#undef GENERATED_DECL
/**
 * Scope for generated functions declarations.
 * May be locally redefined e.g. to make generated functions static
 *
 * @sa GENERATED_DEF
 */
#define GENERATED_DECL extern

#undef GENERATED_DEF
/**
 * Scope for generated functions definitons
 * May be locally redefined e.g. to make generated functions static
 *
 * @sa GENERATED_DEF
 */
#define GENERATED_DEF


