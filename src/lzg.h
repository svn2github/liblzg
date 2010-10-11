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

#ifndef _LIBLZG_H_
#define _LIBLZG_H_

/**
* @file
* @mainpage
*
* @section intro_sec Introduction
*
* liblzg is a minimal implementation of an LZ77 class compression library. The
* main characteristic of the library is that the decoding routine is very
* simple, fast and requires no extra memory (except for the encoded and decoded
* data buffers).
*
* @section funcs_sec Functions
*
* @li LZG_MaxEncodedSize() - Determine the maximum size of the encoded data for
*                            a given uncompressed buffer (worst case).
* @li LZG_Encode() - Encode uncompressed data as LZG coded data.
*
* @li LZG_DecodedSize() - Determine the size of the decoded data for a given
*                         LZG coded buffer.
* @li LZG_Decode() - Decode LZG coded data.
*/

/**
* Determine the size of the decoded data for a given LZG coded buffer.
* @param[in] in Input (compressed) buffer.
* @param[in] insize Size of the input buffer (number of bytes).
* @return The size of the decoded data, or zero if the function failed
*         (e.g. if the data integrity check failed).
*/
unsigned int LZG_DecodedSize(const unsigned char *in, unsigned int insize);


/**
* Decode LZG coded data.
* @param[in]  in Input (compressed) buffer.
* @param[out] out Output (uncompressed) buffer.
* @param[in]  insize Size of the input buffer (number of bytes).
* @param[in]  outsize Size of the output buffer (number of bytes).
* @return The size of the decoded data, or zero if the function failed
*         (e.g. if the end of the output buffer was reached before the
*         entire input buffer was decoded).
*/
unsigned int LZG_Decode(const unsigned char *in, unsigned char *out,
                        unsigned int insize, unsigned int outsize);


#endif // _LIBLZG_H_

