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
*
* @section compr_sec Compression
* Here is a simple example of compressing a data buffer.
*
* @code
*   TBD
* @endcode
*
* @section decompr_sec Decompression
* Here is a simple example of decompressing a compressed data buffer (given
* as buf/bufSize).
*
* @code
*   unsigned char *decBuf;
*   unsigned int decSize;
*
*   // Determine size of decompressed data
*   decSize = LZG_DecodedSize(buf, bufSize);
*   if (decSize)
*   {
*     // Allocate memory for the decompressed data
*     decBuf = (unsigned char*) malloc(decSize);
*     if (decBuf)
*     {
*       // Decompress
*       decSize = LZG_Decode(buf, bufSize, decBuf, decSize);
*       if (decSize)
*       {
*         // Uncompressed data is now in decBuf, use it...
*         // ...
*       }
*       else
*         printf("Decompression failed (bad data)!\n");
*
*       // Free memory when we're done with the decompressed data
*       free(decBuf);
*     }
*     else
*       printf("Out of memory!\n");
*   }
*   else
*     printf("Bad input data!\n");
* @endcode
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
* @param[in]  insize Size of the input buffer (number of bytes).
* @param[out] out Output (uncompressed) buffer.
* @param[in]  outsize Size of the output buffer (number of bytes).
* @return The size of the decoded data, or zero if the function failed
*         (e.g. if the end of the output buffer was reached before the
*         entire input buffer was decoded).
*/
unsigned int LZG_Decode(const unsigned char *in, unsigned int insize,
                        unsigned char *out, unsigned int outsize);


#endif // _LIBLZG_H_

