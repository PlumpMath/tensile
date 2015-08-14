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
 * @brief fast object allocation macros
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef ALLOCATOR_H
#define ALLOCATOR_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include "support.h"

#define DECLARE_TYPE_ALLOCATOR(_type, _args)        \
  extern _type *new_##_type _args                   \
  ATTR_WARN_UNUSED_RESULT ATTR_RETURNS_NONNULL;     \
  extern void free_##_type(_type *_obj);            \
  extern _type *copy_##_type(const _type *_oldobj)

#define DECLARE_REFCNT_ALLOCATOR(_type, _args)  \
  DECLARE_TYPE_ALLOCATOR(_type, _args);         \
  static inline _type *use_##_type(_type *val)  \
  {                                             \
    val->refcnt++;                              \
    return val;                                 \
  }                                             \
  struct fake

#define DECLARE_ARRAY_ALLOCATOR(_type)          \
  DECLARE_TYPE_ALLOCATOR(_type, unsigned n);    \
  extern _type *resize_##_type(_type *arr, unsigned newn)

#define DECLARE_REFCNT_ARRAY_ALLOCATOR(_type)   \
  DECLARE_REFCNT_ALLOCATOR(_type, unsigned n);  \
  extern _type *resize_##_type(_type *arr, unsigned newn)

typedef struct freelist_t {
  struct freelist_t * chain;
} freelist_t;

#define DEFINE_TYPE_ALLOC_COMMON(_type, _args, _var, _init, _clone,  \
                                 _destructor, _fini)                 \
  static freelist_t *freelist_##_type;                               \
  static _type *alloc_##_type(void) ATTR_MALLOC ATTR_RETURNS_NONNULL \
  {                                                                  \
    _type *_var;                                                     \
                                                                     \
    if (freelist_##_type == NULL)                                    \
      _var = malloc(sizeof (*_var));                                 \
    else                                                             \
    {                                                                \
      _var = (_type *)freelist_##_type;                              \
      freelist_##_type = freelist_##_type->chain;                    \
    }                                                                \
    return _var;                                                     \
  }                                                                  \
                                                                     \
  _type *new_##_type _args                                           \
  {                                                                  \
    _type *_var = alloc_##_type();                                   \
    _init;                                                           \
    return _var;                                                     \
  }                                                                  \
                                                                     \
  _type *copy_##_type(const _type *_orig)                            \
  {                                                                  \
    _type *_var = alloc_##_type();                                   \
    memcpy(_var, _orig, sizeof(*_var));                              \
    _clone;                                                          \
    return _var;                                                     \
  }                                                                  \
                                                                     \
  _destructor(_type *_var)                                           \
  {                                                                  \
    _fini;                                                           \
    ((freelist_t *)_var)->chain = freelist_##_type;                  \
    freelist_##_type = (freelist_t *)_var);                          \
  }                                                                  \
  struct fake

#define DEFINE_TYPE_ALLOCATOR(_type, _args, _var, _init, _clone, _fini) \
  DEFINE_TYPE_ALLOC_COMMON(_type, _args, _var, _init, _clone,           \
                           void free_##_type, _fini)


#define DEFINE_REFCNT_ALLOCATOR(_type, _args, _var, _init, _clone, _fini) \
  DEFINE_TYPE_ALLOC_COMMON(_type, _args, _var,                          \
                           { _var->refcnt = 1; _init; },                \
                           { _var->refcnt = 1; _clone; },               \
                           static inline void destroy_##_type, _fini);  \
                                                                        \
  void free_##_type(_type *_var)                                        \
  {                                                                     \
    assert(_var->refcnt != 0);                                          \
    if (--_var->refcnt == 0)                                            \
      destroy_##_type(_var);                                            \
  }

#define DEFINE_ARRAY_ALLOC_COMMON(_type, _getsize, _maxsize, _var, _idxvar, \
                                  _initc, _inite,                       \
                                  _clonec, _clone,                      \
                                  _adjustc, _adjuste                    \
                                  _destructor, _finie, _finic)          \
  static freelist_t *freelists_##_type[_maxsize];                       \
                                                                        \
  static _type *alloc_##_type(unsigned n) ATTR_MALLOC ATTR_RETURNS_NONNULL \
  {                                                                     \
    _type *_var;                                                        \
    unsigned _sz = _getsize(n);                                         \
                                                                        \
    if (_sz >= _maxsize || freelists_##_type[_sz] == NULL)              \
      _var = malloc(sizeof (*_var) + n * sizeof(_var->elts[0]));      \
    else                                                                \
    {                                                                   \
      _var = (_type *)freelists_##_type[_sz];                           \
      freelists_##_type[_sz] = freelists_##_type[_sz]->chain;           \
    }                                                                   \
    _var->nelts = n;                                                    \
    return _var;                                                        \
  }                                                                     \
                                                                        \
  _type *new_##_type(unsigned n)                                        \
  {                                                                     \
    _type *_var = alloc_##_type(n);                                     \
    unsigned _idxvar;                                                   \
                                                                        \
    _initc;                                                             \
    for (_idxvar = 0; _idxvar < n; _idxvar++)                           \
    {                                                                   \
      _inite;                                                           \
    }                                                                   \
    return _var;                                                        \
}                                                                       \
                                                                        \
  _type *copy_##_type(const _type *_orig)                               \
  {                                                                     \
    _type *_var = alloc_##_type(_orig->nelts);                          \
    unsigned _idxvar;                                                   \
                                                                        \
    memcpy(_var, _orig, _orig->nelts * sizeof(_orig->elts[0]));         \
    _clonec;                                                            \
    for (_idxvar = 0; _idxvar < _orig->nelts; _idxvar++)                \
    {                                                                   \
      _clonee;                                                          \
    }                                                                   \
    return _var;                                                        \
  }                                                                     \
                                                                        \
  _type *resize_##_type(_type *_var, unsigned _newn)                    \
  {                                                                     \
    unsigned _idxvar;                                                   \
                                                                        \
    for (_idxvar = _newn; _idxvar < _var->nelts; _idxvar++)             \
    {                                                                   \
      _finie;                                                           \
    }                                                                   \
    if (_getsize(_newn) > _getsize(_var->nelts))                        \
    {                                                                   \
      _type *_fresh = alloc_##_type(_newn);                             \
      memcpy(_fresh, _var, _var->nelts * sizeof(_var->elts[0]));        \
      //for (_idxvar = 0; _idxvar < _fresh->                            \
    }                                                                   \
  }                                                                     \
                                                                        \
  _destructor(_type *_var)                                              \
  {                                                                     \
    unsigned _idxvar;                                                   \
    unsigned _sz = _getsize(_var->nelts);                               \
                                                                        \
    for (_idxvar = 0; _idxvar < _var->nelts; _idxvar++)                 \
    {                                                                   \
      _finie;                                                           \
    }                                                                   \
    _finic;                                                             \
    if(_sz >= _maxsize)                                                 \
      free(_var);                                                       \
    else                                                                \
    {                                                                   \
      ((freelist_t *)_var)->chain = freelists_##_type[_sz];             \
      freelists_##_type[_sz] = (freelist_t *)_var);                     \
  }                                                                     \
  }                                                                     \
  struct fake

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ALLOCATOR_H */
