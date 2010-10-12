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

static int _LZG_SetHeader(const unsigned char *out, unsigned int outsize,
  lzg_header *hdr)
{
    /* Too small buffer? */
    if (outsize < LZG_HEADER_SIZE)
        return FALSE;

    /* FIXME */
    (void) out;
    (void) outsize;
    (void) hdr;

    return TRUE;
}


/*-- PUBLIC ------------------------------------------------------------------*/

unsigned int LZG_MaxEncodedSize(unsigned int insize)
{
    return insize + (insize / 64) + LZG_HEADER_SIZE + 3;
}

unsigned int LZG_Encode(const unsigned char *in, unsigned int insize,
    unsigned char *out, unsigned int outsize)
{
    /* FIXME */
    (void) in;
    (void) insize;
    (void) out;
    (void) outsize;
    lzg_header hdr;
    _LZG_SetHeader(out, outsize, &hdr);

    /* Return size of compressed buffer */
    return 0;
}

