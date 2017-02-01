/**********************************************************************
 * Copyright (c) 2017 Artem V. Andreev
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**********************************************************************/
/** @file
 * @brief common VM types
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef VMTYPES_H
#define VMTYPES_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <inttypes.h>
#include <unitypes.h>
#include <gc/gc.h>
#include <gc/cord.h>
#include <time.h>
#include <rpc/xdr.h>

#include "compiler.h"

enum vm_value_layout {
    VM_VALUE_NONE,
    VM_VALUE_BOOLEAN,
    VM_VALUE_CHARACTER,
    VM_VALUE_INTEGER,
    VM_VALUE_FLOAT,
    VM_VALUE_TIMESTAMP,
    VM_VALUE_STRING,
    VM_VALUE_ARRAY,
    VM_VALUE_BAG,
    VM_VALUE_MAP,
    VM_VALUE_ALGEBRAIC,
    VM_VALUE_TYPE,
    VM_VALUE_SYMBOL,
    VM_VALUE_NODESET,
    VM_VALUE_OPAQUE,
    VM_VALUE_AST
};

typedef struct vm_string_t vm_string_t;
typedef struct vm_array_t vm_array_t;
typedef struct vm_bag_t vm_bag_t;
typedef struct vm_map_t vm_map_t;
typedef struct vm_algebraic_t vm_algebraic_t;
typedef struct vm_nodeset_t vm_nodeset_t;

typedef struct vm_userval_ops_t {
    void *(*init)(void);
    void  (*free)(void *);
    void *(*copy)(const void *);
    bool  (*equal)(const void *, const void *);
    int   (*compare)(const void *, const void *);
    unsigned (*hash)(const void *);
    void  (*encode)(XDR *, const void *);
    void *(*decode)(XDR *);
    CORD  (*pprint)(void *);
} vm_userval_ops_t;

typedef struct vm_userval_t {
    const struct vm_symbol_t *ops;
    void *data;
} vm_userval_t;

typedef struct vm_reference_t {
    const struct vm_module_t *module;
    uintptr_t ref;
} vm_reference_t;

union {
    bool        bval;
    int64_t     ival;
    double      dval;
    time_t      tval;
    ucs4_t      cval;
    CORD        str;
    const union vm_value  *arr;
    const vm_bag_t        *bag;
    const vm_map_t        *map;
    const vm_algebraic_t   *rec;
    const struct vm_type_t *type;
    const struct vm_symbol_t *symbol;
    const vm_nodeset_t     *nodes;
    const vm_userval_t     *opaque;
    const struct vm_ast_t  *ast;
} vm_value;

typedef struct vm_ast_t *(*vm_transformer)(unsigned n,
                                           const struct vm_ast_t *args[]);

typedef struct vm_symbol_t {
    CORD library;
    CORD symbol;
    union {
        vm_transformer func;
        const vm_userval_ops_t *typeops;
    };
} vm_symbol_t;

typedef struct vm_typed_value {
    const struct vm_type_t *type;
    vm_value value;
} vm_typed_value;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* VMTYPES_H */
