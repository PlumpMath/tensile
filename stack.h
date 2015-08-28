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
/**
 * @brief stack operations
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef STACK_H
#define STACK_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include "allocator.h"

#define DECLARE_STACK_TYPE(_type, _eltype) \
    DECLARE_ARRAY_TYPE


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* STACK_H */
