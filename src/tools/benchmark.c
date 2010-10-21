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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lzg.h>

#ifdef USE_ZLIB
# include <zlib.h>
#endif
#ifdef USE_BZ2
# include <bzlib.h>
#endif
#ifdef USE_LZMA
# include <lzma.h>
#endif


/*-- High resolution timer implementation -----------------------------------*/

#ifdef _WIN32
# include <windows.h>
#else
# include <sys/time.h>
#endif

#ifdef _WIN32
static __int64 g_timeStart;
static __int64 g_timeFreq;
#else
static long long g_timeStart;
#endif

void StartTimer(void)
{
#ifdef _WIN32
  if(QueryPerformanceFrequency((LARGE_INTEGER *)&g_timeFreq))
    QueryPerformanceCounter((LARGE_INTEGER *)&g_timeStart);
  else
    g_timeFreq = 0;
#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  g_timeStart = (long long) tv.tv_sec * (long long) 1000000 + (long long) tv.tv_usec;
#endif
}

unsigned int StopTimer(void)
{
#ifdef _WIN32
  __int64 t;
  QueryPerformanceCounter((LARGE_INTEGER *)&t);
  return ((t - g_timeStart) * 1000000) / g_timeFreq;
#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (long long) tv.tv_sec * (long long) 1000000 + (long long) tv.tv_usec - g_timeStart;
#endif
}

/*-- (end of high resolution timer implementation) --------------------------*/


/*-- Dynamic codec class ----------------------------------------------------*/

typedef unsigned int (*MAXENCODEDSIZEFUN)(unsigned int insize);
typedef unsigned int (*ENCODEFUN)(const unsigned char *decBuf, unsigned int decSize,
                                  unsigned char *encBuf, unsigned int maxEncSize,
                                  unsigned int level, LZGPROGRESSFUN progressfun,
                                  void *userdata);
typedef unsigned int (*DECODEFUN)(const unsigned char *encBuf, unsigned int encSize,
                                  unsigned char *decBuf, unsigned int decSize);

typedef struct _codec_t {
    MAXENCODEDSIZEFUN MaxEncodedSize;
    ENCODEFUN         Encode;
    DECODEFUN         Decode;
} codec_t;


static unsigned int LZG_Encode_wrapper(const unsigned char *decBuf,
    unsigned int decSize, unsigned char *encBuf, unsigned int maxEncSize,
    unsigned int level, LZGPROGRESSFUN progressfun, void *userdata)
{
    switch (level)
    {
        case 1: level = LZG_LEVEL_1; break;
        case 2: level = LZG_LEVEL_2; break;
        case 3: level = LZG_LEVEL_3; break;
        case 4: level = LZG_LEVEL_4; break;
        default:
        case 5: level = LZG_LEVEL_5; break;
        case 6: level = LZG_LEVEL_6; break;
        case 7: level = LZG_LEVEL_7; break;
        case 8: level = LZG_LEVEL_8; break;
        case 9: level = LZG_LEVEL_9; break;
    }

    return LZG_Encode(decBuf, decSize, encBuf, maxEncSize, level, 1,
                      progressfun, userdata);
}

static void InitCodecLZG(codec_t *c)
{
    c->MaxEncodedSize = LZG_MaxEncodedSize;
    c->Encode = LZG_Encode_wrapper;
    c->Decode = LZG_Decode;
}

#ifdef USE_ZLIB
static unsigned int ZLIB_MaxEncodedSize_wrapper(unsigned int insize)
{
    // FIXME
    return insize + (insize / 10) + 1000;
}

static unsigned int ZLIB_Encode_wrapper(const unsigned char *decBuf,
    unsigned int decSize, unsigned char *encBuf, unsigned int maxEncSize,
    unsigned int level, LZGPROGRESSFUN progressfun, void *userdata)
{
    int ret;
    unsigned int compressedSize;
    z_stream strm;

    if (progressfun)
        progressfun(0, userdata);

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK) return 0;
    strm.avail_in = decSize;
    strm.next_in = (unsigned char*)decBuf;
    strm.avail_out = maxEncSize;
    strm.next_out = (unsigned char*)encBuf;
    ret = deflate(&strm, Z_FINISH);
    compressedSize = maxEncSize - strm.avail_out;
    deflateEnd(&strm);
    if (ret != Z_STREAM_END) return 0;

    if (progressfun)
        progressfun(100, userdata);

    return compressedSize;
}

static unsigned int ZLIB_Decode_wrapper(const unsigned char *encBuf,
    unsigned int encSize, unsigned char *decBuf, unsigned int decSize)
{
    int ret;
    unsigned int decompressedSize;
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK) return 0;
    strm.avail_in = encSize;
    strm.next_in = (unsigned char*)encBuf;
    strm.avail_out = decSize;
    strm.next_out = (unsigned char*)decBuf;
    ret = inflate(&strm, Z_FINISH);
    decompressedSize = decSize - strm.avail_out;
    inflateEnd(&strm);
    if (ret != Z_STREAM_END) return 0;
    return decompressedSize;
}

static void InitCodecZLIB(codec_t *c)
{
    c->MaxEncodedSize = ZLIB_MaxEncodedSize_wrapper;
    c->Encode = ZLIB_Encode_wrapper;
    c->Decode = ZLIB_Decode_wrapper;
}
#endif

#ifdef USE_BZ2
static unsigned int BZ2_MaxEncodedSize_wrapper(unsigned int insize)
{
    // FIXME
    return insize * 2 + 1000;
}

static unsigned int BZ2_Encode_wrapper(const unsigned char *decBuf,
    unsigned int decSize, unsigned char *encBuf, unsigned int maxEncSize,
    unsigned int level, LZGPROGRESSFUN progressfun, void *userdata)
{
    // FIXME
    return 0;
}

static unsigned int BZ2_Decode_wrapper(const unsigned char *encBuf,
    unsigned int encSize, unsigned char *decBuf, unsigned int decSize)
{
    // FIXME
    return 0;
}

static void InitCodecBZ2(codec_t *c)
{
    c->MaxEncodedSize = BZ2_MaxEncodedSize_wrapper;
    c->Encode = BZ2_Encode_wrapper;
    c->Decode = BZ2_Decode_wrapper;
}
#endif

#ifdef USE_LZMA
static unsigned int LZMA_MaxEncodedSize_wrapper(unsigned int insize)
{
    // FIXME
    return insize * 2 + 1000;
}

static unsigned int LZMA_Encode_wrapper(const unsigned char *decBuf,
    unsigned int decSize, unsigned char *encBuf, unsigned int maxEncSize,
    unsigned int level, LZGPROGRESSFUN progressfun, void *userdata)
{
    // FIXME
    return 0;
}

static unsigned int LZMA_Decode_wrapper(const unsigned char *encBuf,
    unsigned int encSize, unsigned char *decBuf, unsigned int decSize)
{
    // FIXME
    return 0;
}

static void InitCodecLZMA(codec_t *c)
{
    c->MaxEncodedSize = LZMA_MaxEncodedSize_wrapper;
    c->Encode = LZMA_Encode_wrapper;
    c->Decode = LZMA_Decode_wrapper;
}
#endif

/*-- (end of dynamic codec class) -------------------------------------------*/


void ShowUsage(char *prgName)
{
    fprintf(stderr, "Usage: %s [options] file\n", prgName);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, " -1    Use fastest compression\n");
    fprintf(stderr, " -9    Use best compression\n");
    fprintf(stderr, " -v    Be verbose\n");
    fprintf(stderr, " -lzg  Use LZG compression (default).\n");
#ifdef USE_ZLIB
    fprintf(stderr, " -zlib Use zlib compression.\n");
#endif
#ifdef USE_BZ2
    fprintf(stderr, " -bz2  Use bzip2 compression.\n");
#endif
#ifdef USE_LZMA
    fprintf(stderr, " -lzma Use lzma compression.\n");
#endif
    fprintf(stderr, "\nDescription:\n");
    fprintf(stderr, "This program will load the given file, compress it, and then decompress it\n");
    fprintf(stderr, "again. The time it takes to do the operations are measured (excluding file\n");
    fprintf(stderr, "I/O etc), and printed to stdout.\n");
}

void ShowProgress(int progress, void *data)
{
    FILE *f = (FILE *)data;
    fprintf(f, "Progress: %d%%   \r", progress);
    fflush(f);
}

int main(int argc, char **argv)
{
    char *inName;
    FILE *inFile;
    size_t fileSize;
    unsigned char *decBuf;
    unsigned int decSize = 0;
    unsigned char *encBuf;
    unsigned int maxEncSize, encSize, level, verbose, t;
    int arg;
    LZGPROGRESSFUN progressfun = 0;
    codec_t c;

    // Default arguments
    inName = NULL;
    level = 5;
    verbose = 0;
    InitCodecLZG(&c);

    // Get arguments
    for (arg = 1; arg < argc; ++arg)
    {
        if (strcmp("-1", argv[arg]) == 0)
            level = 1;
        else if (strcmp("-2", argv[arg]) == 0)
            level = 2;
        else if (strcmp("-3", argv[arg]) == 0)
            level = 3;
        else if (strcmp("-4", argv[arg]) == 0)
            level = 4;
        else if (strcmp("-5", argv[arg]) == 0)
            level = 5;
        else if (strcmp("-6", argv[arg]) == 0)
            level = 6;
        else if (strcmp("-7", argv[arg]) == 0)
            level = 7;
        else if (strcmp("-8", argv[arg]) == 0)
            level = 8;
        else if (strcmp("-9", argv[arg]) == 0)
            level = 9;
        else if (strcmp("-v", argv[arg]) == 0)
            verbose = 1;
        else if (strcmp("-lzg", argv[arg]) == 0)
            InitCodecLZG(&c);
#ifdef USE_ZLIB
        else if (strcmp("-zlib", argv[arg]) == 0)
            InitCodecZLIB(&c);
#endif
#ifdef USE_BZ2
        else if (strcmp("-bz2", argv[arg]) == 0)
            InitCodecBZ2(&c);
#endif
#ifdef USE_LZMA
        else if (strcmp("-lzma", argv[arg]) == 0)
            InitCodecLZMA(&c);
#endif
        else if (!inName)
            inName = argv[arg];
        else
        {
            ShowUsage(argv[0]);
            return 0;
        }
    }
    if (!inName)
    {
        ShowUsage(argv[0]);
        return 0;
    }

    // Read input file
    decBuf = (unsigned char*) 0;
    inFile = fopen(inName, "rb");
    if (inFile)
    {
        fseek(inFile, 0, SEEK_END);
        fileSize = (unsigned int) ftell(inFile);
        fseek(inFile, 0, SEEK_SET);
        if (fileSize > 0)
        {
            decSize = (unsigned int) fileSize;
            decBuf = (unsigned char*) malloc(decSize);
            if (decBuf)
            {
                if (fread(decBuf, 1, decSize, inFile) != decSize)
                {
                    fprintf(stderr, "Error reading \"%s\".\n", inName);
                    free(decBuf);
                    decBuf = (unsigned char*) 0;
                }
            }
            else
                fprintf(stderr, "Out of memory.\n");
        }
        else
            fprintf(stderr, "Input file is empty.\n");

        fclose(inFile);
    }
    else
        fprintf(stderr, "Unable to open file \"%s\".\n", inName);

    if (!decBuf)
        return 0;

    // Determine maximum size of compressed data
    maxEncSize = c.MaxEncodedSize(decSize);

    // Allocate memory for the compressed data
    encBuf = (unsigned char*) malloc(maxEncSize);
    if (encBuf)
    {
        // Compress
        if (verbose)
            progressfun = ShowProgress;
        StartTimer();
        encSize = c.Encode(decBuf, decSize, encBuf, maxEncSize,
                            level, progressfun, stderr);
        t = StopTimer();
        if (encSize)
        {
            fprintf(stdout, "Compression: %d us (%lld KB/s)\n", t,
                            (decSize * (long long) 977) / t);

            // Compressed data is now in encBuf, now decompress it...
            StartTimer();
            decSize = c.Decode(encBuf, encSize, decBuf, decSize);
            t = StopTimer();
            if (decSize)
            {
                fprintf(stdout, "Decompression: %d us (%lld KB/s)\n", t,
                                (decSize * (long long) 977) / t);
                fprintf(stdout, "Sizes: %d => %d bytes, %d%%\n", decSize, encSize,
                                (100 * encSize) / decSize);
            }
            else
                fprintf(stderr, "Decompression failed!\n");
        }
        else
            fprintf(stderr, "Compression failed!\n");

        // Free memory when we're done with the compressed data
        free(encBuf);
    }
    else
        fprintf(stderr, "Out of memory!\n");

    // Free memory
    free(decBuf);

    return 0;
}

