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

#ifdef __cplusplus
extern "C" {
#endif

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
* Here is a simple example of compressing an uncompressed data buffer (given
* as buf/bufSize).
*
* @code
*     unsigned char *encBuf;
*     unsigned int encSize, maxEncSize;
*
*     // Determine maximum size of compressed data
*     maxEncSize = LZG_MaxEncodedSize(bufSize);
*
*     // Allocate memory for the compressed data
*     encBuf = (unsigned char*) malloc(maxEncSize);
*     if (encBuf)
*     {
*         // Compress
*         encSize = LZG_Encode(buf, bufSize, encBuf, maxEncSize, LZG_LEVEL_DEFAULT, NULL, NULL);
*         if (encSize)
*         {
*             // Compressed data is now in encBuf, use it...
*             // ...
*         }
*         else
*             fprintf(stderr, "Compression failed!\n");
*
*         // Free memory when we're done with the compressed data
*         free(encBuf);
*     }
*     else
*         fprintf(stderr, "Out of memory!\n");
* @endcode
*
* @section decompr_sec Decompression
* Here is a simple example of decompressing a compressed data buffer (given
* as buf/bufSize).
*
* @code
*     unsigned char *decBuf;
*     unsigned int decSize;
*
*     // Determine size of decompressed data
*     decSize = LZG_DecodedSize(buf, bufSize);
*     if (decSize)
*     {
*         // Allocate memory for the decompressed data
*         decBuf = (unsigned char*) malloc(decSize);
*         if (decBuf)
*         {
*             // Decompress
*             decSize = LZG_Decode(buf, bufSize, decBuf, decSize);
*             if (decSize)
*             {
*                 // Uncompressed data is now in decBuf, use it...
*                 // ...
*             }
*             else
*                 printf("Decompression failed (bad data)!\n");
*
*             // Free memory when we're done with the decompressed data
*             free(decBuf);
*         }
*         else
*             printf("Out of memory!\n");
*     }
*     else
*         printf("Bad input data!\n");
* @endcode
*/

#define LZG_LEVEL_1 1032    /**< @brief Lowest/fastest compression level */
#define LZG_LEVEL_2 2048    /**< @brief Compression level 2 */
#define LZG_LEVEL_3 4096    /**< @brief Compression level 3 */
#define LZG_LEVEL_4 8192    /**< @brief Compression level 4 */
#define LZG_LEVEL_5 16384   /**< @brief Medium compression level */
#define LZG_LEVEL_6 32768   /**< @brief Compression level 6 */
#define LZG_LEVEL_7 65536   /**< @brief Compression level 7 */
#define LZG_LEVEL_8 131072  /**< @brief Compression level 8 */
#define LZG_LEVEL_9 262152  /**< @brief Best/slowest compression level */

/** @brief Default compression level */
#define LZG_LEVEL_DEFAULT LZG_LEVEL_5

/**
* Determine the maximum size of the encoded data for a given uncompressed
* buffer.
* @param[in] insize Size of the uncompressed buffer (number of bytes).
* @return Worst case (maximum) size of the encoded data.
*/
unsigned int LZG_MaxEncodedSize(unsigned int insize);

/**
* Progress callback function.
* @param[in] progress The current progress (0-100).
* @param[in] userdata User supplied data pointer.
*/
typedef void (*LZGPROGRESSFUN)(int progress, void *userdata);

/**
* Encode uncompressed data using the LZG coder (i.e. compress the data).
* @param[in]  in Input (uncompressed) buffer.
* @param[in]  insize Size of the input buffer (number of bytes).
* @param[out] out Output (compressed) buffer.
* @param[in]  outsize Size of the output buffer (number of bytes).
* @param[in]  window Compression search window (bigger is better and slower).
*             For convenience, you can use the predefined constants
*             @ref LZG_LEVEL_1 (fast) to @ref LZG_LEVEL_9 (slow), or
*             @ref LZG_LEVEL_DEFAULT.
* @param[in]  progressfun Encoding progress callback function (set this to NULL
*             to disable progress callback).
* @param[in]  userdata User data pointer for the progress callback function
*             (this can set to NULL).
* @return The size of the encoded data, or zero if the function failed
*         (e.g. if the end of the output buffer was reached before the
*         entire input buffer was encoded).
*/
unsigned int LZG_Encode(const unsigned char *in, unsigned int insize,
                        unsigned char *out, unsigned int outsize,
                        unsigned int window, LZGPROGRESSFUN progressfun,
                        void *userdata);


/**
* Determine the size of the decoded data for a given LZG coded buffer.
* @param[in] in Input (compressed) buffer.
* @param[in] insize Size of the input buffer (number of bytes). This does
*            not have to be the size of the entire compressed data, but
*            it has to be at least 7 bytes (the first few bytes of the
*            header, including the decompression size).
* @return The size of the decoded data, or zero if the function failed
*         (e.g. if the magic header ID could not be found).
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

#ifdef __cplusplus
}
#endif

#endif // _LIBLZG_H_
