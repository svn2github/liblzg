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
#include <lzg.h>

int main(int argc, char **argv)
{
    FILE *inFile, *outFile;
    size_t fileSize;
    unsigned char *decBuf;
    unsigned int decSize;
    unsigned char *encBuf;
    unsigned int maxEncSize, encSize;
    int useStdout = 0;

    // Check arguments
    if ((argc < 2) || (argc > 3))
    {
        fprintf(stderr, "Usage: %s infile [outfile]\n", argv[0]);
        fprintf(stderr, "If no output file is given, stdout is used for output.\n");
        return 0;
    }

    // Use stdout?
    if (argc < 3)
        useStdout = 1;

    // Read input file
    decBuf = (unsigned char*) 0;
    inFile = fopen(argv[1], "rb");
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
                    fprintf(stderr, "Error reading \"%s\".\n", argv[1]);
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
        fprintf(stderr, "Unable to open file \"%s\".\n", argv[1]);

    if (!decBuf)
        return 0;

    // Determine maximum size of compressed data
    maxEncSize = LZG_MaxEncodedSize(decSize);

    // Allocate memory for the decompressed data
    encBuf = (unsigned char*) malloc(maxEncSize);
    if (decBuf)
    {
        // Compress
        encSize = LZG_Encode(decBuf, decSize, encBuf, maxEncSize);
        if (encSize)
        {
            // Compressed data is now in encBuf, write it...
            if (!useStdout)
            {
                outFile = fopen(argv[2], "wb");
                if (!outFile)
                    fprintf(stderr, "Unable to open file \"%s\".\n", argv[2]);
            }
            else
                outFile = stdout;

            if (outFile)
            {
                // Write data
                if (fwrite(encBuf, 1, encSize, outFile) != encSize)
                    fprintf(stderr, "Error writing to output file.\n");

                // Close file
                if (!useStdout)
                    fclose(outFile);
            }
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

