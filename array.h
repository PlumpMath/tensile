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
 * @brief generic array operations
 *
 * @author Artem V. Andreev <artem@AA5779.spb.edu>
 */
#ifndef ARRAY_H
#define ARRAY_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#define DECLARE_ARRAY_OPS(_type, _eltype)                               \
    typedef void (*_type##_mapper)(const _eltype *src, _eltype *dst);   \
    typedef void (*_type##_zipper)(const _eltype *src1,                 \
                                   const _eltype *src2,                 \
                                   _eltype *dst);                       \
    typedef void (*_type##_folder)(void *accum, const _eltype *item);   \
    extern _type *map_##_type(_type##_mapper fn, const _type *arg)      \
        ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT;                       \
    extern _type *zip_##_type(_type##_mapper fn,                        \
                              const _type *arg1,                        \
                              const _type *arg2)                        \
        ATTR_NONNULL_1ST ATTR_WARN_UNUSED_RESULT;                       \
    extern void fold_##_type(_type##_folder fn, void *accum,            \
                             const _type *arg)                          \
        ATTR_NONNULL_1ST



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ARRAY_H */
