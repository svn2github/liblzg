/* -*- mode: c; tab-width: 4; indent-tabs-mode: nil; -*- */

/*
* This file is part of liblzg.
*
* Copyright (c) 2010 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*/

#include "internal.h"


/*-- PRIVATE -----------------------------------------------------------------*/

static int _LZG_GetHeader(const unsigned char *in, unsigned int insize,
  lzg_header *hdr)
{
    /* Bad input buffer? */
    if (insize < LZG_HEADER_SIZE)
        return FALSE;

    /* Check magic number */
    if ((in[0] != 'L') || (in[1] != 'Z') || (in[2] != 'G'))
        return FALSE;

    /* Check which method is used */
    hdr->method = in[3];
    if (hdr->method > LZG_METHOD_LZG1)
        return FALSE;

    /* Get & check input buffer size */
    hdr->encodedSize = (((unsigned int)in[4]) << 24) |
                       (((unsigned int)in[5]) << 16) |
                       (((unsigned int)in[6]) << 8) |
                       ((unsigned int)in[7]);
    if (hdr->encodedSize != (insize - LZG_HEADER_SIZE))
        return FALSE;

    /* Get output buffer size */
    hdr->decodedSize = (((unsigned int)in[8]) << 24) |
                       (((unsigned int)in[9]) << 16) |
                       (((unsigned int)in[10]) << 8) |
                       ((unsigned int)in[11]);

    /* Get & check checksum */
    hdr->checksum = (((unsigned int)in[12]) << 24) |
                    (((unsigned int)in[13]) << 16) |
                    (((unsigned int)in[14]) << 8) |
                    ((unsigned int)in[15]);
    if (_LZG_CalcChecksum(&in[LZG_HEADER_SIZE], hdr->encodedSize) != hdr->checksum)
        return FALSE;

    return TRUE;
}


/*-- PUBLIC ------------------------------------------------------------------*/

unsigned int LZG_DecodedSize(const unsigned char *in, unsigned int insize)
{
    lzg_header hdr;

    /* Get/check header info (including checksum operation) */
    if (!_LZG_GetHeader(in, insize, &hdr))
        return 0;

    return hdr.decodedSize;
}

unsigned int LZG_Decode(const unsigned char *in, unsigned int insize,
    unsigned char *out, unsigned int outsize)
{
    unsigned char *src, *in_end, *dst, *out_end, *copy, symbol, b;
    unsigned char copy3marker, copy4marker, copyNmarker;
    unsigned int  i, length, offset;
    lzg_header hdr;

    /* Do we have anything to uncompress? */
    if (insize < (LZG_HEADER_SIZE + 3))
        return 0;

    /* Get/check header info */
    if (!_LZG_GetHeader(in, insize, &hdr))
        return 0;

    /* Enough space in the output buffer? */
    if (outsize < hdr.decodedSize)
        return 0;

    /* Initialize the byte streams */
    src = (unsigned char *)in;
    in_end = ((unsigned char *)in) + insize;
    dst = out;
    out_end = out + outsize;

    /* Skip header information */
    src += LZG_HEADER_SIZE;

    /* Plain copy? */
    if (hdr.method == LZG_METHOD_COPY)
    {
        if (hdr.decodedSize != hdr.encodedSize)
            return 0;

        /* Copy 1:1, input buffer to output buffer */
        for (i = hdr.decodedSize - 1; i > 0; --i)
            *dst++ = *src++;

        return hdr.decodedSize;
    }

    /* Get marker symbols from the input stream */
    copy3marker = *src++;
    copy4marker = *src++;
    copyNmarker = *src++;

    /* Main decompression loop */
    while (src < in_end)
    {
        if (dst >= out_end) return 0;

        /* Get the next symbol */
        symbol = *src++;

        /* Copy marker */
        if ((symbol == copy3marker) ||
            (symbol == copy4marker) ||
            (symbol == copyNmarker))
        {
            if (src >= in_end) return 0;
            b = *src++;

            if (b > 0)
            {
                /* Decode offset / length parameters */
                if (symbol == copy3marker)
                {
                    /* Copy 3 bytes */
                    length = 3;

                    /* Offset is in the range 1..254 */
                    offset = (unsigned int) b;
                }
                else if  (symbol == copy4marker)
                {
                    /* Copy 4 bytes */
                    length = 4;

                    /* Offset is in the range 1..254 */
                    offset = (unsigned int) b;
                }
                else
                {
                    /* Copy at least 4 bytes */
                    length = (unsigned int) b + 3;

                    /* Decode offset using varying size coding:
                       1-128:         1 byte
                       129-16384:     2 bytes
                       16385-4194304: 3 bytes
                    */
                    b = *src++;
                    offset = (unsigned int) (b & 0x7f);
                    if (b >= 0x80)
                    {
                        if (src >= in_end) return 0;
                        b = *src++;;
                        offset = (offset << 7) | (unsigned int) (b & 0x7f);
                        if (b >= 0x80)
                        {
                            if (src >= in_end) return 0;
                            b = *src++;;
                            offset = (offset << 8) | (unsigned int) b;
                        }
                    }
                    offset++;
                }

                if ((dst + length) > out_end) return 0;

                /* Copy corresponding data from history window */
                copy = dst - offset;
                if (copy < out) return 0;
                for (i = 0; i < length; ++i)
                    *dst++ = *copy++;
            }
            else
            {
                /* ...single occurance of the marker symbol */
                *dst++ = symbol;
            }
        }

        /* No marker, plain copy... */
        else
        {
            *dst++ = symbol;
        }
    }

    /* Did we get the right number of output bytes? */
    if ((dst - out) != hdr.decodedSize)
        return 0;

    /* Return size of decompressed buffer */
    return hdr.decodedSize;
}

