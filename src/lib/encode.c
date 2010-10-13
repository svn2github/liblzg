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

#include <stdlib.h>
#include <string.h>
#include "internal.h"


/*-- PRIVATE -----------------------------------------------------------------*/

#define LZG_MAX_OFFSET 100000


static void _LZG_SetHeader(unsigned char *out, lzg_header *hdr)
{
    /* Magic number */
    out[0] = 'L';
    out[1] = 'Z';
    out[2] = 'G';

    /* Input buffer size */
    out[3] = hdr->encodedSize >> 24;
    out[4] = hdr->encodedSize >> 16;
    out[5] = hdr->encodedSize >> 8;
    out[6] = hdr->encodedSize;

    /* Output buffer size */
    out[7] = hdr->decodedSize >> 24;
    out[8] = hdr->decodedSize >> 16;
    out[9] = hdr->decodedSize >> 8;
    out[10] = hdr->decodedSize;

    /* Checksum */
    hdr->checksum = _LZG_CalcChecksum(&out[LZG_HEADER_SIZE], hdr->encodedSize);
    out[11] = hdr->checksum >> 24;
    out[12] = hdr->checksum >> 16;
    out[13] = hdr->checksum >> 8;
    out[14] = hdr->checksum;

    /* Method */
    out[15] = hdr->method;
}

typedef struct _hist_rec {
    unsigned int count;
    unsigned int symbol;
} hist_rec;

static int hist_rec_compare(const void *p1, const void *p2)
{
    return ((hist_rec*)p1)->count - ((hist_rec*)p2)->count;
}

static int _LZG_DetermineMarkers(const unsigned char *in, unsigned int insize,
    unsigned char *leastCommon1, unsigned char *leastCommon2,
    unsigned char *leastCommon3)
{
    hist_rec *hist;
    unsigned int i;
    unsigned char *src;

    /* Allocate memory for histogram */
    hist = (hist_rec*) malloc(sizeof(hist_rec) * 256);
    if (!hist)
        return FALSE;

    /* Build histogram, O(n) */
    for (i = 0; i < 256; ++i)
    {
        hist[i].count = 0;
        hist[i].symbol = i;
    }
    src = (unsigned char *) in;
    for (i = 0; i < insize; ++i)
        hist[*src++].count++;

    /* Sort histogram */
    qsort((void *)hist, 256, sizeof(hist_rec), hist_rec_compare);

    /* Find least common symbols */
    *leastCommon1 = (unsigned char) hist[0].symbol;
    *leastCommon2 = (unsigned char) hist[1].symbol;
    *leastCommon3 = (unsigned char) hist[2].symbol;

    /* Free memory for histogram */
    free(hist);

    return TRUE;
}

static unsigned int _LZG_FindMatch(const unsigned char *first,
  const unsigned char *end, const unsigned char *pos, unsigned int maxOffset,
  unsigned int *offset)
{
    unsigned int length, bestLength = 2;
    int win, bestWin = 0;
    unsigned char *cmp1, *cmp2;
    unsigned int i;

    /* Try all offsets */
    *offset = 0;
    for (i = 1; i <= maxOffset; ++i)
    {
        /* Calculate maximum match length for this offset */
        cmp1 = (unsigned char*) pos;
        cmp2 = cmp1 - i;
        if (cmp2 < first) break;
        length = 0;
        while ((cmp1 < end) && (*cmp1++ == *cmp2++) && (length < 255)) ++length;

        /* Improvement in match length? */
        if (length > bestLength)
        {
            /* Get actual compression win for this match */
            if ((length <= 4) && (i <= 255))
                win = length - 2;
            else
            {
                win = length - 3;
                if (i >= 129) --win;
                if (i >= 16385) --win;
            }

            /* Best so far? */
            if (win > bestWin)
            {
                bestWin = win;
                *offset = i;
                bestLength = length;
            }
        }
    }

    /* Did we get a match that would actually compress? */
    if (bestWin > 0)
        return bestLength;
    else
        return 0;
}


/*-- PUBLIC ------------------------------------------------------------------*/

unsigned int LZG_MaxEncodedSize(unsigned int insize)
{
    return insize + ((insize + 63) / 64) + LZG_HEADER_SIZE + 3;
}

unsigned int LZG_Encode(const unsigned char *in, unsigned int insize,
    unsigned char *out, unsigned int outsize)
{
    unsigned char *src, *in_end, *dst, *out_end, symbol;
    unsigned char copy3marker, copy4marker, copyNmarker;
    unsigned int length, offset = 0;
    lzg_header hdr;

    /* Too small output buffer? */
    if (outsize < LZG_HEADER_SIZE)
        return 0;

    /* Calculate histogram and find optimal marker symbols */
    if (!_LZG_DetermineMarkers(in, insize, &copy3marker, &copy4marker, &copyNmarker))
        return 0;

    /* Initialize the byte streams */
    src = (unsigned char *)in;
    in_end = ((unsigned char *)in) + insize;
    dst = out;
    out_end = out + outsize;

    /* Skip header information */
    dst += LZG_HEADER_SIZE;

    /* Set marker symbols */
    if ((dst + 3) > out_end) return 0;
    *dst++ = copy3marker;
    *dst++ = copy4marker;
    *dst++ = copyNmarker;

    /* Main compression loop */
    while (src < in_end)
    {
        if (dst >= out_end) return 0;

        /* Find best history match for this position in the input buffer */
        length = _LZG_FindMatch(in, in_end, src, LZG_MAX_OFFSET, &offset);
        if (length > 0)
        {
            if ((length == 3) && (offset <= 255))
            {
                /* Copy 3 bytes, short offset */
                if ((dst + 2) > out_end) return 0;
                *dst++ = copy3marker;
                *dst++ = offset;
            }
            else if ((length == 4) && (offset <= 255))
            {
                /* Copy 4 bytes, short offset */
                if ((dst + 2) > out_end) return 0;
                *dst++ = copy4marker;
                *dst++ = offset;
            }
            else
            {
                /* Copy variable number of bytes, any offset */
                if ((dst + 2) > out_end) return 0;
                *dst++ = copyNmarker;
                *dst++ = length - 3;
                --offset;
                if (offset >= 16384)
                {
                    if ((dst + 3) > out_end) return 0;
                    *dst++ = (offset >> 15) | 0x80;
                    *dst++ = (offset >> 8) | 0x80;
                    *dst++ = offset;
                }
                else if (offset >= 128)
                {
                    if ((dst + 2) > out_end) return 0;
                    *dst++ = (offset >> 7) | 0x80;
                    *dst++ = offset & 0x7f;
                }
                else
                {
                    if (dst >= out_end) return 0;
                    *dst++ = offset;
                }
            }
            src += length;
        }
        else
        {
            /* Plain copy */
            symbol = *src++;
            *dst++ = symbol;

            /* Was this symbol equal to any of the markers? */
            if ((symbol == copy3marker) ||
                (symbol == copy4marker) ||
                (symbol == copyNmarker))
            {
                if (dst >= out_end) return 0;
                *dst++ = 0;
            }
        }
    }

    /* Set header data */
    hdr.method = LZG_METHOD_LZG1;
    hdr.encodedSize = (dst - out) - LZG_HEADER_SIZE;
    hdr.decodedSize = insize;
    _LZG_SetHeader(out, &hdr);

    /* Return size of compressed buffer */
    return LZG_HEADER_SIZE + hdr.encodedSize;
}

