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
#include "lzg.h"

int main(int argc, char **argv)
{
    FILE *inFile, *outFile;
    unsigned char *encBuf;
    unsigned int encSize;
    unsigned char *decBuf;
    unsigned int decSize;

    // Check arguments
    if (argc != 3)
    {
        printf("Usage: %s infile outfile\n", argv[0]);
        return 0;
    }

    // Read input file
    printf("Loading from \"%s\".\n", argv[1]);
    encBuf = (unsigned char*) 0;
    inFile = fopen(argv[1], "rb");
    if (inFile)
    {
        fseek(inFile, 0, SEEK_END);
        encSize = (unsigned int) ftell(inFile);
        fseek(inFile, 0, SEEK_SET);
        if (encSize > 0)
        {
            encBuf = (unsigned char*) malloc(encSize);
            if (encBuf)
            {
                if (fread(encBuf, 1, encSize, inFile) != encSize)
                {
                    printf("Error reading \"%s\".\n", argv[1]);
                    free(encBuf);
                    encBuf = (unsigned char*) 0;
                }
            }
            else
                printf("Out of memory.\n");
        }
        else
            printf("Input file is empty.\n");

        fclose(inFile);
    }
    else
        printf("Unable to open file \"%s\".\n", argv[1]);

    if (!encBuf)
        return 0;

    // Determine size of decompressed data
    decSize = LZG_DecodedSize(encBuf, encSize);
    if (decSize)
    {
        // Allocate memory for the decompressed data
        decBuf = (unsigned char*) malloc(decSize);
        if (decBuf)
        {
            // Decompress
            printf("Decompressing to %d bytes.", decSize);
            decSize = LZG_Decode(encBuf, encSize, decBuf, decSize);
            if (decSize)
            {
                // Uncompressed data is now in decBuf, write it...
                printf("Saving to \"%s\".\n", argv[2]);
                outFile = fopen(argv[2], "wb");
                if (outFile)
                {
                    // Write data
                    if (fwrite(decBuf, 1, decSize, outFile) != decSize)
                        printf("Error writing \"%s\".\n", argv[2]);

                    // Close file
                    fclose(outFile);
                }
                else
                    printf("Unable to open file \"%s\".\n", argv[2]);
            }
            else
                printf("Decompression failed (bad data)!\n");

            // Free memory when we're done with the decompressed data
            free(decBuf);
        }
        else
            printf("Out of memory!\n");
    }
    else
        printf("Bad input data!\n");

    // Free memory
    free(encBuf);

    return 0;
}

