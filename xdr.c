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
