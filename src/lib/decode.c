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

#define _LZG_GetUINT32(in, offs) \
    ((((unsigned int)in[offs]) << 24) | \
     (((unsigned int)in[offs+1]) << 16) | \
     (((unsigned int)in[offs+2]) << 8) | \
     ((unsigned int)in[offs+3]))

/* LUT for decoding the copy length parameter */
static const unsigned char _LZG_LENGTH_LUT[34] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,35,48,72,128
};


/*-- PUBLIC ------------------------------------------------------------------*/

unsigned int LZG_DecodedSize(const unsigned char *in, unsigned int insize)
{
    if (insize < 7)
        return 0;

    /* Check magic number */
    if ((in[0] != 'L') || (in[1] != 'Z') || (in[2] != 'G'))
        return 0;

    /* Get output buffer size */
    return _LZG_GetUINT32(in, 3);
}

unsigned int LZG_Decode(const unsigned char *in, unsigned int insize,
    unsigned char *out, unsigned int outsize)
{
    unsigned char *src, *inEnd, *dst, *outEnd, *copy, symbol, b;
    unsigned char marker1, marker2, marker3;
    unsigned int  i, length, offset;
    unsigned int  encodedSize, decodedSize;
    unsigned int  checksum;
    unsigned char method;

    /* Does the input buffer at least contain the header? */
    if (insize < LZG_HEADER_SIZE)
        return 0;

    /* Check magic number */
    if ((in[0] != 'L') || (in[1] != 'Z') || (in[2] != 'G'))
        return 0;

    /* Get & check output buffer size */
    decodedSize = _LZG_GetUINT32(in, 3);
    if (outsize < decodedSize)
        return 0;

    /* Get & check input buffer size */
    encodedSize = _LZG_GetUINT32(in, 7);
    if (encodedSize != (insize - LZG_HEADER_SIZE))
        return 0;

    /* Get & check checksum */
    checksum = _LZG_GetUINT32(in, 11);
    if (_LZG_CalcChecksum(&in[LZG_HEADER_SIZE], encodedSize) != checksum)
        return 0;

    /* Check which method is used */
    method = in[15];
    if (method > LZG_METHOD_LZG1)
        return 0;

    /* Initialize the byte streams */
    src = (unsigned char *)in;
    inEnd = ((unsigned char *)in) + insize;
    dst = out;
    outEnd = out + outsize;

    /* Skip header information */
    src += LZG_HEADER_SIZE;

    /* Plain copy? */
    if (method == LZG_METHOD_COPY)
    {
        if (decodedSize != encodedSize)
            return 0;

        /* Copy 1:1, input buffer to output buffer */
        for (i = decodedSize - 1; i > 0; --i)
            *dst++ = *src++;

        return decodedSize;
    }

    /* Get marker symbols from the input stream */
    if ((src + 3) > inEnd) return 0;
    marker1 = *src++;
    marker2 = *src++;
    marker3 = *src++;

    /* Main decompression loop */
    while (src < inEnd)
    {
        /* Get the next symbol */
        symbol = *src++;

        /* Marker symbol? */
        if ((symbol == marker1) ||
            (symbol == marker2) ||
            (symbol == marker3))
        {
            if (src >= inEnd) return 0;
            b = *src++;

            if (b)
            {
                /* Decode offset / length parameters */
                if (symbol == marker1)
                {
                    /* Short copy */
                    length = (b >> 6) + 3;
                    offset = (b & 0x3f) + 8;
                }
                else if (symbol == marker2)
                {
                    /* Near copy */
                    length = _LZG_LENGTH_LUT[(b & 0x1f) + 2];
                    offset = (b >> 5) + 1;
                }
                else
                {
                    /* Generic copy */
                    length = _LZG_LENGTH_LUT[(b & 0x1f) + 2];
                    offset = ((unsigned int) (b & 0xe0)) << 2;

                    /* Decode offset using varying size coding:
                       1-1024:         +1 byte
                       1025-262144:    +2 bytes
                    */
                    if (src >= inEnd) return 0;
                    b = *src++;
                    offset |= b & 0x7f;
                    if (b >= 0x80)
                    {
                        if (src >= inEnd) return 0;
                        offset = (offset << 8) | (*src++);
                    }
                    offset += 8;
                }

                /* Copy corresponding data from history window */
                copy = dst - offset;
                if ((copy < out) || ((dst + length) > outEnd)) return 0;
                for (i = 0; i < length; ++i)
                    *dst++ = *copy++;

                continue;
            }
        }

        /* Plain copy (or single occurance of a marker symbol)... */
        if (dst >= outEnd) return 0;
        *dst++ = symbol;
    }

    /* Did we get the right number of output bytes? */
    if ((unsigned int)(dst - out) != decodedSize)
        return 0;

    /* Return size of decompressed buffer */
    return decodedSize;
}
