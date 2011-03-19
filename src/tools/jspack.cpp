/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; -*- */

/*
* This file is part of liblzg.
*
* Copyright (c) 2011 Marcus Geelnard
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

#include <iostream>
#include <fstream>
#include <ostream>
#include <istream>

#include <stdlib.h>
#include <string.h>
#include <lzg.h>

using namespace std;


void ShowProgress(int progress, void *data)
{
    ostream *o = (ostream *)data;
    (*o) << "Progress: " << progress << "%   \r" << flush;
}

void ShowUsage(char *prgName)
{
    cerr << "Usage: " << prgName << " [options] infile [outfile]" << endl;
    cerr << endl << "Options:" << endl;
    cerr << " -v        Be verbose" << endl;
    cerr << " -nostrip  Do not strip/preprocess JavaScript source" << endl;
    cerr << " -V        Show LZG library version and exit" << endl;
    cerr << endl << "If no output file is given, stdout is used for output." << endl;
}

int main(int argc, char **argv)
{
    char *inName, *outName;
    size_t fileSize;
    unsigned char *decBuf;
    lzg_uint32_t decSize = 0;
    unsigned char *encBuf;
    lzg_uint32_t maxEncSize, encSize;
    int arg, verbose, strip;
    lzg_encoder_config_t config;

    // Default arguments
    inName = NULL;
    outName = NULL;
    LZG_InitEncoderConfig(&config);
    config.fast = LZG_TRUE;
    config.level = LZG_LEVEL_9;
    verbose = 0;
    strip = 1;

    // Get arguments
    for (arg = 1; arg < argc; ++arg)
    {
        if (strcmp("-v", argv[arg]) == 0)
            verbose = 1;
        else if (strcmp("-nostrip", argv[arg]) == 0)
            strip = 0;
        else if (strcmp("-V", argv[arg]) == 0)
        {
            cout << "LZG library version " << LZG_VersionString() << endl;
            return 0;
        }
        else if (!inName)
            inName = argv[arg];
        else if (!outName)
            outName = argv[arg];
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
    ifstream inFile(inName, ios_base::in | ios_base::binary);
    if (!inFile.fail())
    {
        inFile.seekg(0, ios::end);
        fileSize = (size_t) inFile.tellg();
        inFile.seekg(0, ios::beg);
        if (fileSize > 0)
        {
            decSize = (lzg_uint32_t) fileSize;
            decBuf = (unsigned char*) malloc(decSize);
            if (decBuf)
            {
                inFile.read((char*)decBuf, decSize);
                if (inFile.gcount() != decSize)
                {
                    cerr << "Error reading \"" << inName << "\"." << endl;
                    free(decBuf);
                    decBuf = (unsigned char*) 0;
                }
            }
            else
                cerr << "Out of memory." << endl;
        }
        else
            cerr << "Input file is empty." << endl;

        inFile.close();
    }
    else
        cerr << "Unable to open file \"" << inName << "\"." << endl;

    if (!decBuf)
        return 0;

    // Strip whitespaces etc...
    if (strip)
    {
        // FIXME
    }

    // Determine maximum size of compressed data
    maxEncSize = LZG_MaxEncodedSize(decSize);

    // Allocate memory for the compressed data
    encBuf = (unsigned char*) malloc(maxEncSize);
    if (encBuf)
    {
        // Compress
        if (verbose)
        {
            config.progressfun = ShowProgress;
            config.userdata = (void*)&cerr;
        }
        encSize = LZG_Encode(decBuf, decSize, encBuf, maxEncSize, &config);
        if (encSize)
        {
            if (verbose)
            {
                cerr << "Result: " << encSize << " bytes (" <<
                        ((100 * encSize) / decSize) << "% of the original)" << endl;
            }

            // Encode in printable characters...
            // FIXME!

            // Compressed data is now in encBuf, write it...
            bool failed = false;
            if (outName)
            {
                ofstream outFile(outName, ios_base::out | ios_base::binary);
                if (outFile.fail())
                    cerr << "Unable to open file \"" << outName << "\"." << endl;
                outFile.write((char*)encBuf, encSize);
                failed = outFile.fail();
                outFile.close();
            }
            else
            {
                cout.write((char*)encBuf, encSize);
                failed = cout.fail();
            }
            if (failed)
              cerr << "Error writing to output file." << endl;
        }
        else
            cerr << "Compression failed!" << endl;

        // Free memory when we're done with the compressed data
        free(encBuf);
    }
    else
        cerr << "Out of memory!" << endl;

    // Free memory
    free(decBuf);

    return 0;
}

