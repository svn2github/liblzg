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


/*-- PRIVATE -----------------------------------------------------------------*/

#define LZG_HEADER_SIZE 16

typedef struct _lzg_header {
  unsigned int encodedSize;
  unsigned int decodedSize;
  unsigned int checksum;
} lzg_header;

unsigned int _LZG_CalcChecksum(const unsigned char *in, unsigned int insize);

static int _LZG_GetHeader(const unsigned char *in, unsigned int insize,
  lzg_header *hdr)
{
    /* Bad input buffer? */
    if (insize < LZG_HEADER_SIZE)
        return 0;

    /* Check magic number */
    if ((in[0] != 'L') || (in[1] != 'Z') || (in[2] != 'G') || (in[3] != 1))
        return 0;

    /* Get & check input buffer size */
    hdr->encodedSize = (((unsigned int)in[4]) << 24) |
                       (((unsigned int)in[5]) << 16) |
                       (((unsigned int)in[6]) << 8) |
                       ((unsigned int)in[7]);
    if (hdr->encodedSize != (insize - LZG_HEADER_SIZE))
        return 0;

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
        return 0;

    return 1;
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

unsigned int LZG_Decode(const unsigned char *in, unsigned char *out,
    unsigned int insize, unsigned int outsize)
{
    unsigned char *in_pos, *in_end, *out_pos, *out_end, *copy, symbol, b;
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
    in_pos = (unsigned char *)in;
    in_end = ((unsigned char *)in) + insize;
    out_pos = out;
    out_end = out + outsize;

    /* Skip header information */
    in_pos += LZG_HEADER_SIZE;

    /* Get marker symbols from the input stream */
    copy3marker = *in_pos++;
    copy4marker = *in_pos++;
    copyNmarker = *in_pos++;

    /* Main decompression loop */
    while (in_pos < in_end)
    {
        if (out_pos >= out_end) return 0;

        /* Get the next symbol */
        symbol = *in_pos++;

        /* COPY-3/COPY-4-marker: Copy three/four bytes */
        if ((symbol == copy3marker) || (symbol == copy4marker))
        {
            /* Get offset (1..255) */
            if (in_pos >= in_end) return 0;
            offset = *in_pos++;
            if (offset > 0)
            {
                /* Copy three/four bytes */
                if (symbol == copy3marker)
                  length = 3;
                else
                  length = 4;
                if ((out_pos + length) > out_end) return 0;

                /* Copy corresponding data from history window */
                copy = out_pos - offset;
                if (copy < out) return 0;
                for (i = 0; i < length; ++i)
                    *out_pos++ = *copy++;
            }
            else
            {
                /* ...single occurance of the marker symbol */
                *out_pos++ = symbol;
            }
        }

        /* COPY-N-marker: Copy variable number of bytes */
        else if (symbol == copyNmarker)
        {
            /* Get length */
            if (in_pos >= in_end) return 0;
            length = *in_pos++;
            if (length > 0)
            {
                /* Copy at least four bytes (4..258) */
                length += 3;
                if ((out_pos + length) > out_end) return 0;

                /* Decode offset using varying size coding:
                   1-128:         1 byte
                   129-16384:     2 bytes
                   16385-4194304: 3 bytes
                */
                b = *in_pos++;
                offset = (unsigned int) (b & 0x7f);
                if (b >= 0x80)
                {
                    if (in_pos >= in_end) return 0;
                    b = *in_pos++;;
                    offset = (offset << 7) | (unsigned int) (b & 0x7f);
                    if (b >= 0x80)
                    {
                        if (in_pos >= in_end) return 0;
                        b = *in_pos++;;
                        offset = (offset << 8) | (unsigned int) b;
                    }
                }
                offset++;

                /* Copy corresponding data from history window */
                copy = out_pos - offset;
                if (copy < out) return 0;
                for (i = 0; i < length; ++i)
                    *out_pos++ = *copy++;
            }
            else
            {
                /* ...single occurance of the marker symbol */
                *out_pos++ = symbol;
            }
        }

        /* No marker, plain copy... */
        else
        {
            *out_pos++ = symbol;
        }
    }

    /* Did we get the right number of output bytes? */
    if ((out_pos - out) != hdr.decodedSize)
        return 0;

    /* Return size of decompressed buffer */
    return hdr.decodedSize;
}

