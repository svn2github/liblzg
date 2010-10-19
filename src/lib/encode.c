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

#define _LZG_MAX_RUN_LENGTH 257

/* Memory usage is affected by this define as follows:

   [32-bit systems]
   _LZG_USE_FASTEST_METHOD undefined => (65536 + window)*4 bytes
   _LZG_USE_FASTEST_METHOD defined   => (16777216 + window)*4 bytes

   [64-bit systems]
   _LZG_USE_FASTEST_METHOD undefined => (65536 + window)*8 bytes
   _LZG_USE_FASTEST_METHOD defined   => (16777216 + window)*8 bytes
*/
#define _LZG_USE_FASTEST_METHOD

#ifdef _LZG_USE_FASTEST_METHOD
# define _LZG_ACCEL_LAST_BUF 16777216
#else
# define _LZG_ACCEL_LAST_BUF 65536
#endif


static void _LZG_SetHeader(unsigned char *out, lzg_header *hdr)
{
    /* Magic number */
    out[0] = 'L';
    out[1] = 'Z';
    out[2] = 'G';

    /* Decoded buffer size */
    out[3] = hdr->decodedSize >> 24;
    out[4] = hdr->decodedSize >> 16;
    out[5] = hdr->decodedSize >> 8;
    out[6] = hdr->decodedSize;

    /* Encoded buffer size */
    out[7] = hdr->encodedSize >> 24;
    out[8] = hdr->encodedSize >> 16;
    out[9] = hdr->encodedSize >> 8;
    out[10] = hdr->encodedSize;

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
    unsigned char *leastCommon3, unsigned char *leastCommon4)
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
    *leastCommon4 = (unsigned char) hist[3].symbol;

    /* Free memory for histogram */
    free(hist);

    return TRUE;
}

typedef struct _search_accel {
    unsigned char **tab;
    unsigned char **last;
    size_t window;
    size_t size;
} search_accel;

static search_accel* _LZG_SearchAccel_Create(size_t window, size_t size)
{
    size_t i;
    search_accel *self;

    /* Allocate memory for the sarch tab object */
    self = malloc(sizeof(search_accel));
    if (!self)
        return (search_accel*) 0;

    /* Allocate memory for the table */
    self->tab = malloc(sizeof(unsigned char *) * window);
    if (!self->tab)
    {
        free(self);
        return (search_accel*) 0;
    }

    /* Allocate memory for the "last symbol occurance" array */
    self->last = malloc(sizeof(unsigned char *) * _LZG_ACCEL_LAST_BUF);
    if (!self->last)
    {
        free(self->tab);
        free(self);
        return (search_accel*) 0;
    }

    /* Init (clear) arrays */
    self->window = window;
    self->size = size;
    for (i = 0; i < window; ++i)
        self->tab[i] = (unsigned char*) 0;
    for (i = 0; i < _LZG_ACCEL_LAST_BUF; ++i)
        self->last[i] = (unsigned char*) 0;

    return self;
}

static void _LZG_SearchAccel_Destroy(search_accel *self)
{
    if (!self)
        return;

    free(self->last);
    free(self->tab);
    free(self);
}

static void _LZG_UpdateLastPos(search_accel *sa,
    const unsigned char *first, unsigned char *pos)
{
    unsigned int lIdx;

    if (((size_t)(pos - first) + 2) >= sa->size) return;
#ifdef _LZG_USE_FASTEST_METHOD
    lIdx = (((unsigned int)pos[0]) << 16) |
           (((unsigned int)pos[1]) << 8) |
           ((unsigned int)pos[2]);
#else
    lIdx = (((unsigned int)pos[0]) << 8) |
           ((unsigned int)pos[1]);
#endif
    sa->tab[(pos - first) % sa->window] = sa->last[lIdx];
    sa->last[lIdx] = pos;
}

static unsigned int _LZG_FindMatch(search_accel *sa, const unsigned char *first,
  const unsigned char *end, const unsigned char *pos, size_t window,
  size_t symbolCost, size_t *offset)
{
    size_t length, bestLength = 2, dist;
    int win, bestWin = 0;
    unsigned char *pos2, *cmp1, *cmp2, *minPos;

    *offset = 0;

    /* Minimum search position */
    if ((size_t)(pos - first) >= window)
        minPos = (unsigned char*)(pos - window);
    else
        minPos = (unsigned char*)first;

    /* Previous search position */
    pos2 = sa->tab[(pos - first) % window];

    /* Main search loop */
    while (pos2 && (pos2 > minPos))
    {
        /* Calculate maximum match length for this offset */
        cmp1 = (unsigned char*)pos + 3;
        cmp2 = pos2 + 3;
        length = 3;
        while ((cmp1 < end) && (*cmp1++ == *cmp2++) && (length < _LZG_MAX_RUN_LENGTH))
            ++length;

        /* Improvement in match length? */
        if (length > bestLength)
        {
            dist = (size_t)(pos - pos2);

            /* Get actual compression win for this match */
            if ((dist == 1) || ((length <= 4) && (dist <= 255)))
                win = symbolCost + length - 3;
            else
            {
                win = symbolCost + length - 4;
                if (dist >= 129) --win;
                if (dist >= 16385) --win;
            }

            /* Best so far? */
            if (win > bestWin)
            {
                bestWin = win;
                *offset = dist;
                bestLength = length;
                if (length == _LZG_MAX_RUN_LENGTH)
                    break;
            }
        }

        /* Previous search position */
        pos2 = sa->tab[(pos2 - first) % window];
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
    return insize + ((insize + 63) / 64) + LZG_HEADER_SIZE + 4;
}

unsigned int LZG_Encode(const unsigned char *in, unsigned int insize,
    unsigned char *out, unsigned int outsize, unsigned int window,
    LZGPROGRESSFUN progressfun, void *userdata)
{
    unsigned char *src, *inEnd, *dst, *outEnd, symbol;
    unsigned char copy3Marker, copy4Marker, copyNMarker, rleMarker;
    size_t length, offset = 0, symbolCost, i;
    int isMarkerSymbol, progress, oldProgress = -1;
    search_accel *sa = (search_accel*) 0;
    lzg_header hdr;

    /* Check arguments */
    if ((!in) || (!out) || (window < 10) || (outsize < LZG_HEADER_SIZE))
        goto fail;

    /* Calculate histogram and find optimal marker symbols */
    if (!_LZG_DetermineMarkers(in, insize, &copy3Marker, &copy4Marker,
                               &copyNMarker, &rleMarker))
        goto fail;

    /* Initialize search accelerator */
    sa = _LZG_SearchAccel_Create(window, insize);
    if (!sa)
        goto fail;

    /* Initialize the byte streams */
    src = (unsigned char *)in;
    inEnd = ((unsigned char *)in) + insize;
    dst = out;
    outEnd = out + outsize;

    /* Skip header information */
    dst += LZG_HEADER_SIZE;

    /* Set marker symbols */
    if ((dst + 4) > outEnd) goto fail;
    *dst++ = copy3Marker;
    *dst++ = copy4Marker;
    *dst++ = copyNMarker;
    *dst++ = rleMarker;

    /* Try LZG compression first - if it grows too much, fall back to copy */
    hdr.method = LZG_METHOD_LZG1;

    /* Main compression loop */
    while (src < inEnd)
    {
        /* Report progress? */
        if (progressfun)
        {
            progress = (100 * (src - in)) / insize;
            if (progress != oldProgress)
            {
                progressfun(progress, userdata);
                oldProgress = progress;
            }
        }

        /* Is the compressed data larger than the uncompressed data? */
        if ((dst - out) > (inEnd - in))
        {
            /* Fall back to copy */
            hdr.method = LZG_METHOD_COPY;
            src = (unsigned char *)in;
            dst = out + LZG_HEADER_SIZE;
            while (src < inEnd)
                *dst++ = *src++;
            break;
        }

        /* Update search accelerator */
        _LZG_UpdateLastPos(sa, in, src);

        /* Get current symbol (don't increment, yet) */
        symbol = *src;

        /* Is this a marker symbol? */
        isMarkerSymbol = (symbol == copy3Marker) ||
                         (symbol == copy4Marker) ||
                         (symbol == copyNMarker) ||
                         (symbol == rleMarker);

        /* What's the cost for this symbol if we do not compress */
        symbolCost = isMarkerSymbol ? 2 : 1;

        /* Find best history match for this position in the input buffer */
        length = _LZG_FindMatch(sa, in, inEnd, src, window, symbolCost,
                                &offset);

        if (length > 0)
        {
            if (offset == 1)
            {
                /* Special case: RLE */
                if ((dst + 2) > outEnd) goto fail;
                *dst++ = rleMarker;
                *dst++ = length - 2;
            }
            else if ((length == 3) && (offset <= 255))
            {
                /* Copy 3 bytes, short offset */
                if ((dst + 2) > outEnd) goto fail;
                *dst++ = copy3Marker;
                *dst++ = offset;
            }
            else if ((length == 4) && (offset <= 255))
            {
                /* Copy 4 bytes, short offset */
                if ((dst + 2) > outEnd) goto fail;
                *dst++ = copy4Marker;
                *dst++ = offset;
            }
            else
            {
                /* Copy variable number of bytes, any offset */
                if ((dst + 2) > outEnd) goto fail;
                *dst++ = copyNMarker;
                *dst++ = length - 2;
                --offset;
                if (offset >= 16384)
                {
                    if ((dst + 3) > outEnd) goto fail;
                    *dst++ = (offset >> 15) | 0x80;
                    *dst++ = (offset >> 8) | 0x80;
                    *dst++ = offset;
                }
                else if (offset >= 128)
                {
                    if ((dst + 2) > outEnd) goto fail;
                    *dst++ = (offset >> 7) | 0x80;
                    *dst++ = offset & 0x7f;
                }
                else
                {
                    if (dst >= outEnd) goto fail;
                    *dst++ = offset;
                }
            }

            /* Skip ahead (and update search accelerator)... */
            for (i = 1; i < length; ++i)
                _LZG_UpdateLastPos(sa, in, src + i);
            src += length;
        }
        else
        {
            /* Plain copy */
            if (dst >= outEnd) goto fail;
            *dst++ = symbol;
            ++src;

            /* Was this symbol equal to any of the markers? */
            if (isMarkerSymbol)
            {
                if (dst >= outEnd) goto fail;
                *dst++ = 0;
            }
        }
    }

    /* Report progress? (we're done now) */
    if (progressfun)
        progressfun(100, userdata);

    /* Set header data */
    hdr.encodedSize = (dst - out) - LZG_HEADER_SIZE;
    hdr.decodedSize = insize;
    _LZG_SetHeader(out, &hdr);

    /* Return size of compressed buffer */
    return LZG_HEADER_SIZE + hdr.encodedSize;

    /* Exit routine for failure situations */
fail:
    if (sa)
        _LZG_SearchAccel_Destroy(sa);
    return 0;
}

