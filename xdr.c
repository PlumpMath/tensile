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
 * @author Artem V. Andreev <artem@iling.spb.ru>
 */

#include <arpa/inet.h>
#include "xdr.h"

tn_status
tn_xdr_read_from_stream(tn_xdr_stream * restrict stream,
                        void * restrict dest, size_t sz)
{
    tn_status status;
    
    if (stream->pos + sz > stream->limit)
        return ENOMSG;

    status = stream->reader(stream, dest, sz);
    if (status == 0)
        stream->pos += sz;

    return status;
}

tn_status
tn_xdr_write_to_stream(tn_xdr_stream * restrict stream,
                       const void * restrict dest, size_t sz)
{
    tn_status status;
    
    if (stream->pos + sz > stream->limit)
        return ENOSPC;

    status = stream->reader(stream, dest, sz);
    if (status == 0)
        stream->pos += sz;

    return status;
}


tn_status
tn_xdr_encode_int64(tn_xdr_stream * restrict stream,
                    const int64_t * restrict data)
{
    tn_status rc = 0;
    int32_t high = (int32_t)(*data >> 32);
    uint32_t low = (uint32_t)(*data & 0xffffffffu);

    rc = tn_xdr_encode_int32(stream, &high);
    if (rc == 0)
        rc = tn_xdr_encode_uint32(stream, &low);
}

tn_status
tn_xdr_decode_int64(tn_xdr_stream * restrict stream, int64_t * restrict data)
{
    tn_status rc = 0;
    int32_t high;
    uint32_t low;

    rc = tn_xdr_decode_int32(stream, &high);
    if (rc == 0)
        rc = tn_xdr_decode_uint32(stream, &low);
    if (rc == 0)
        *data = ((int64_t)high << 32) | low;

    return rc;
}

tn_status
tn_xdr_encode_uint64(tn_xdr_stream * restrict stream,
                    const uint64_t * restrict data)
{
    tn_status rc = 0;
    uint32_t high = (int32_t)(*data >> 32);
    uint32_t low = (uint32_t)(*data & 0xffffffffu);

    rc = tn_xdr_encode_uint32(stream, &high);
    if (rc == 0)
        rc = tn_xdr_encode_uint32(stream, &low);
}

tn_status
tn_xdr_decode_uint64(tn_xdr_stream * restrict stream, uint64_t * restrict data)
{
    tn_status rc = 0;
    uint32_t high;
    uint32_t low;

    rc = tn_xdr_decode_uint32(stream, &high);
    if (rc == 0)
        rc = tn_xdr_decode_uint32(stream, &low);
    if (rc == 0)
        *data = ((uint64_t)high << 32) | low;

    return rc;
}

warn_unused_result warn_any_null_arg
static tn_status
xdr_add_padding(tn_xdr_stream * stream, size_t len)
{
    uint32_t zero = 0;

    if (len % 4 == 0)
        return 0;

    return tn_xdr_write_to_stream(stream, &zero, len % 4);
}


warn_unused_result warn_any_null_arg
static tn_status
xdr_skip_padding(tn_xdr_stream * stream, size_t len)
{
    tn_status rc;
    uint32_t scratch = 0;

    if (len % 4 == 0)
        return 0;

    rc = tn_xdr_read_from_stream(stream, &zero, len % 4);
    if (rc != 0)
        return rc;

    return scratch == 0 ? 0 : EPROTO;
}


tn_status
tn_xdr_encode_bytes(tn_xdr_stream * restrict stream,
                    size_t len,
                    const uint8_t data[restrict var_size(len)])
{
    tn_status rc =
        tn_xdr_write_to_stream(stream, len, data);

    if (rc == 0)
        rc = xdr_add_padding(stream, len);

    return rc;
}

tn_status
tn_xdr_decode_bytes(tn_xdr_stream * restrict stream,
                    size_t len,
                    uint8_t data[restrict var_size(len)])
{
    tn_status rc =
        tn_xdr_read_from_stream(stream, len, data);

    if (rc == 0)
        rc = xdr_skip_padding(stream, len);

    return rc;
}

tn_status
tn_xdr_encode_var_bytes(tn_xdr_stream * restrict stream,
                        size_t len,
                        const uint8_t data[restrict var_size(len)])
{
    tn_status rc;
    uint32_t len32;
    
    if (len > UINT32_MAX)
        return EOVERFLOW;
    
    len32 = (uint32_t)len;
    rc = tn_xdr_encode_uint32(stream, &len32);
    if (rc != 0)
        return rc;

    return tn_xdr_encode_bytes(stream, len, data);
}

warn_unused_result warn_any_null_arg
static tn_status
xdr_decode_length(tn_xdr_stream * restrict stream,
                  size_t * restrict length)
{
    uint32_t len32;
    tn_status rc = tn_xdr_decode_uint32(stream, &len32);

    if (rc != 0)
        return rc;

    if (stream->pos + len32 > stream->limit ||
        stream->pos + len32 < stream->pos)
        return EPROTO;

    *length = (size_t)len32;
    return 0;
}

tn_status
tn_xdr_decode_var_bytes(tn_xdr_stream * restrict stream,
                        size_t * restrict len,
                        uint8_t ** restrict data)
{
    tn_status rc;
    size_t declen;
    uint8_t *buf;

    rc = xdr_decode_length(stream, &declen);
    if (rc != 0)
        return rc;
    if (declen == 0)
    {
        *len = 0;
        *data = NULL;
        return 0;
    }

    buf = te_alloc_blob(declen);
    rc = tn_xdr_decode_bytes(stream, len32, buf);
    if (rc != 0)
        return rc;
    
    *len  = declen;
    *data = buf;
    return 0;
}
