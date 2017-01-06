/*
 * Copyright (c) 2016  Artem V. Andreev
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
 * @brief Abstract Syntax Tree
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef AST_H
#define AST_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include "vmtypes.h"

enum ast_node_kind {
    AST_LITERAL,
    AST_REFERENCE,
    AST_APPLY,
    AST_LAMBDA,
    AST_FORCE,
};

typedef struct ast_argument {
    CORD name;
    bool implicit;
    struct ast_node *child;
} ast_argument;

typedef struct ast_node {
    enum ast_node_kind kind;
    union {
        vm_typed_value literal;
        CORD ref;
        struct {
            struct ast_node *functor;
            CORD name;
            struct ast_node *arg;
        } apply;
        struct {
            struct ast_node *body;
            unsigned n_args;
            ast_argument args[];
        } lambda;
        struct ast_node *delay;
    };
} ast_node;


extern ast_node *ast_create_node(enum ast_node_kind kind, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* AST_H */
