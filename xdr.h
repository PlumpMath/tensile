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
 * @brief XDR implementation
 *
 * @author Artem V. Andreev <artem@iling.spb.ru>
 */
#ifndef XDR_H
#define XDR_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include "compiler.h"
#include "status.h"

typedef struct tn_xdr_stream {
    tn_status (*reader)(struct tn_xdr_stream * restrict,
                        void * restrict , size_t);
    tn_status (*writer)(struct tn_xdr_stream * restrict,
                        const void * restrict, size_t);

    void *data;
    size_t pos;
    size_t limit;
} tn_xdr_stream;

warn_unused_result
warn_null_args
extern tn_status tn_xdr_read_from_stream(tn_xdr_stream * restrict stream,
                                         void * restrict dest, size_t sz);


warn_unused_result
warn_null_args
extern tn_status tn_xdr_write_to_stream(tn_xdr_stream * restrict stream,
                                        const void * restrict dest,
                                        size_t sz);

warn_unused_result
warn_null_args
static inline tn_status
tn_xdr_mem_reader(tn_xdr_stream * restrict stream, void * restrict dest,
                  size_t sz)
{
    memcpy(dest, stream->data, sz);
    stream->data = (uint8_t *)stream->data + sz;
    return 0;
}

warn_unused_result
warn_null_args
static inline tn_status
tn_xdr_mem_writer(tn_xdr_stream * restrict stream, void * restrict dest,
                  size_t sz)
{
    memcpy(stream->data, dest, sz);
    stream->data = (uint8_t *)stream->data + sz;
    return 0;
}

#define TN_XDR_STREAM_STATIC_ARRAY(_array) \
    ((tn_xdr_stream) {                     \
            .reader = tn_xdr_mem_reader,   \
            .writer = tn_xdr_mem_writer,   \
            .data = (_array),              \
            .pos = 0,                      \
            .limit = sizeof(_array)        \
            })

typedef tn_status (*tn_xdr_encoder)(tn_xdr_stream * restrict,
                                    const void * restrict);

typedef tn_status (*tn_xdr_decoder)(tn_xdr_stream * restrict,
                                    void * restrict);

extern tn_status tn_xdr_encode_int32(tn_xdr_stream * restrict,
                                     const int32_t * restrict);

extern tn_status tn_xdr_decode_int32(tn_xdr_stream * restrict,
                                     int32_t * restrict);

extern tn_status tn_xdr_encode_uint32(tn_xdr_stream * restrict,
                                      const uint32_t * restrict);

extern tn_status tn_xdr_decode_uint32(tn_xdr_stream * restrict,
                                      uint32_t * restrict);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* XDR_H */
