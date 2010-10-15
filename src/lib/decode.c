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

    /* Get output buffer size */
    hdr->decodedSize = (((unsigned int)in[3]) << 24) |
                       (((unsigned int)in[4]) << 16) |
                       (((unsigned int)in[5]) << 8) |
                       ((unsigned int)in[6]);

    /* Get & check input buffer size */
    hdr->encodedSize = (((unsigned int)in[7]) << 24) |
                       (((unsigned int)in[8]) << 16) |
                       (((unsigned int)in[9]) << 8) |
                       ((unsigned int)in[10]);
    if (hdr->encodedSize != (insize - LZG_HEADER_SIZE))
        return FALSE;

    /* Get & check checksum */
    hdr->checksum = (((unsigned int)in[11]) << 24) |
                    (((unsigned int)in[12]) << 16) |
                    (((unsigned int)in[13]) << 8) |
                    ((unsigned int)in[14]);
    if (_LZG_CalcChecksum(&in[LZG_HEADER_SIZE], hdr->encodedSize) != hdr->checksum)
        return FALSE;

    /* Check which method is used */
    hdr->method = in[15];
    if (hdr->method > LZG_METHOD_LZG1)
        return FALSE;

    return TRUE;
}


/*-- PUBLIC ------------------------------------------------------------------*/

unsigned int LZG_DecodedSize(const unsigned char *in, unsigned int insize)
{
    if (insize < 7)
        return 0;

    /* Check magic number */
    if ((in[0] != 'L') || (in[1] != 'Z') || (in[2] != 'G'))
        return FALSE;

    /* Get output buffer size */
    return (((unsigned int)in[3]) << 24) |
           (((unsigned int)in[4]) << 16) |
           (((unsigned int)in[5]) << 8) |
           ((unsigned int)in[6]);
}

unsigned int LZG_Decode(const unsigned char *in, unsigned int insize,
    unsigned char *out, unsigned int outsize)
{
    unsigned char *src, *inEnd, *dst, *outEnd, *copy, symbol, b;
    unsigned char copy3Marker, copy4Marker, copyNMarker, rleMarker;
    unsigned int  i, length, offset;
    lzg_header hdr;

    /* Get/check header info */
    if (!_LZG_GetHeader(in, insize, &hdr))
        return 0;

    /* Enough space in the output buffer? */
    if (outsize < hdr.decodedSize)
        return 0;

    /* Initialize the byte streams */
    src = (unsigned char *)in;
    inEnd = ((unsigned char *)in) + insize;
    dst = out;
    outEnd = out + outsize;

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
    if ((src + 4) > inEnd) return 0;
    copy3Marker = *src++;
    copy4Marker = *src++;
    copyNMarker = *src++;
    rleMarker = *src++;

    /* Main decompression loop */
    while (src < inEnd)
    {
        /* Get the next symbol */
        symbol = *src++;

        /* Copy marker */
        if ((symbol == copy3Marker) ||
            (symbol == copy4Marker) ||
            (symbol == copyNMarker) ||
            (symbol == rleMarker))
        {
            if (src >= inEnd) return 0;
            b = *src++;

            if (b > 0)
            {
                /* Decode offset / length parameters */
                if (symbol == copy3Marker)
                {
                    /* Copy 3 bytes */
                    length = 3;

                    /* Offset is in the range 1..255 */
                    offset = (unsigned int) b;
                }
                else if (symbol == copy4Marker)
                {
                    /* Copy 4 bytes */
                    length = 4;

                    /* Offset is in the range 1..255 */
                    offset = (unsigned int) b;
                }
                else if (symbol == copyNMarker)
                {
                    /* Copy at least 3 bytes */
                    length = (unsigned int) b + 2;

                    /* Decode offset using varying size coding:
                       1-128:         1 byte
                       129-16384:     2 bytes
                       16385-4194304: 3 bytes
                    */
                    if (src >= inEnd) return 0;
                    b = *src++;
                    offset = (unsigned int) (b & 0x7f);
                    if (b >= 0x80)
                    {
                        if (src >= inEnd) return 0;
                        b = *src++;;
                        offset = (offset << 7) | (unsigned int) (b & 0x7f);
                        if (b >= 0x80)
                        {
                            if (src >= inEnd) return 0;
                            b = *src++;;
                            offset = (offset << 8) | (unsigned int) b;
                        }
                    }
                    offset++;
                }
                else /* rleMarker */
                {
                    /* Copy at least 2 bytes (excluding first symbol) */
                    length = (unsigned int) b + 1;

                    /* Offset is 1 (start copying at the first occurance of the
                       repeat symbol) */
                    offset = 1;

                    /* Put the repeat symbol in the output buffer */
                    if ((src >= inEnd) || (dst >= outEnd)) return 0;
                    *dst++ = *src++;
                }

                /* Copy corresponding data from history window */
                copy = dst - offset;
                if ((copy < out) || ((dst + length) > outEnd)) return 0;
                for (i = 0; i < length; ++i)
                    *dst++ = *copy++;
            }
            else
            {
                /* ...single occurance of the marker symbol */
                if (dst >= outEnd) return 0;
                *dst++ = symbol;
            }
        }

        /* No marker, plain copy... */
        else
        {
            if (dst >= outEnd) return 0;
            *dst++ = symbol;
        }
    }

    /* Did we get the right number of output bytes? */
    if ((unsigned int)(dst - out) != hdr.decodedSize)
        return 0;

    /* Return size of decompressed buffer */
    return hdr.decodedSize;
}

