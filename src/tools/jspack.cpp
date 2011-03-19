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
#include <string>

#include <stdlib.h>
#include <lzg.h>

using namespace std;

// List of characters that do not require a white space
static const char collapseChars[] = {
    '{', '}', '(', ')', '[', ']', '<', '>', '=', '+', '-', '*', '/', '%', '!',
    ',', '~', '&', '|', ':', ';'
};

lzg_uint32_t StripSource(unsigned char *buf, lzg_uint32_t size)
{
    lzg_uint32_t src, dst = 0;
    bool inComment1 = false, inComment2 = false;
    bool inString1 = false, inString2 = false;
    bool hasWhitespace = false;
    for (src = 0; src < size; ++src)
    {
        bool keep = false;
        if (!(inComment1 || inComment2))
        {
            if (inString1 || inString2)
            {
                // Has the string been terminated?
                if (inString1 && (buf[src] == 34))
                    inString1 = false;
                else if (inString2 && (buf[src] == 39))
                    inString2 = false;
                keep = true;

                hasWhitespace = false;
            }
            else
            {
                if (buf[src] == 34)
                {
                    // Start of string type 1 (starting with a ")
                    inString1 = true;
                    keep = true;
                }
                else if (buf[src] == 39)
                {
                    // Start of string type 2 (starting with a ')
                    inString2 = true;
                    keep = true;
                }
                else if ((src < size - 1) && (buf[src] == '/') &&
                         (buf[src+1] == '/'))
                {
                    // Start of comment type 1 (line comment)
                    inComment1 = true;
                }
                else if ((src < size - 1) && (buf[src] == '/') &&
                         (buf[src+1] == '*'))
                {
                    // Start of comment type 2 (block comment)
                    inComment2 = true;
                }
                else if ((buf[src] == 9) || (buf[src] == 32))
                {
                    // This is a white space, should we keep it?
                    if (!hasWhitespace)
                    {
                        buf[src] = 32;
                        keep = true;

                        // Can we collapse with the previous char?
                        if (src > 1)
                        {
                            for (unsigned int i = 0; i < sizeof(collapseChars); ++i)
                            {
                                if (buf[src-1] == collapseChars[i])
                                    keep = false;
                            }
                        }

                        // Can we collapse with the next char?
                        if (src < size - 1)
                        {
                            for (unsigned int i = 0; i < sizeof(collapseChars); ++i)
                            {
                                if (buf[src+1] == collapseChars[i])
                                    keep = false;
                            }
                        }
                    }
                }
                else if ((buf[src] != 10) && (buf[src] != 13))
                {
                    // None white space char - keep it!
                    keep = true;
                }

                hasWhitespace = (buf[src] == 9) || (buf[src] == 32);
            }
        }
        else
        {
            // Has the comment been terminated?
            if (inComment1)
            {
                if ((buf[src] == 10) || (buf[src] == 13))
                    inComment1 = false;
            }
            else if (inComment2 && (src >= 1))
            {
                if ((buf[src-1] == '*') && (buf[src] == '/'))
                    inComment2 = false;
            }

            hasWhitespace = false;
        }

        if (keep)
           buf[dst++] = buf[src];
    }

    return dst;
}

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
    cerr << " -nosfx    Do not create a self extracting JavaScript program" << endl;
    cerr << " -V        Show LZG library version and exit" << endl;
    cerr << endl << "If no output file is given, stdout is used for output." << endl;
}

int main(int argc, char **argv)
{
    // Default arguments
    char *inName = NULL, *outName = NULL;
    lzg_encoder_config_t config;
    LZG_InitEncoderConfig(&config);
    config.fast = LZG_TRUE;
    config.level = LZG_LEVEL_9;
    bool verbose = false;
    bool strip = true;
    bool sfx = true;

    // Get arguments
    for (int i = 1; i < argc; ++i)
    {
        string arg(argv[i]);
        if (arg == "-v")
            verbose = true;
        else if (arg == "-nostrip")
            strip = false;
        else if (arg == "-nosfx")
            sfx = false;
        else if (arg == "-V")
        {
            cout << "LZG library version " << LZG_VersionString() << endl;
            return 0;
        }
        else if (!inName)
            inName = argv[i];
        else if (!outName)
            outName = argv[i];
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
    size_t fileSize = 0;
    unsigned char *decBuf;
    lzg_uint32_t decSize = 0;
    unsigned char *encBuf;
    lzg_uint32_t maxEncSize, encSize;
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

    if (verbose)
    {
        cout << "Original size:       " << decSize << " bytes" << endl;
    }

    // Strip whitespaces etc...
    if (strip)
    {
        // Do several passes, until there is nothing more to strip
        lzg_uint32_t decSizeOld;
        do {
            decSizeOld = decSize;
            decSize = StripSource(decBuf, decSize);
        } while (decSize != decSizeOld);

        if (verbose)
        {
            cout << "Stripped size:       " << decSize << " bytes (" <<
                    (100 * decSize) / fileSize << "% of the original)" << endl;
        }
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
                cerr << "Binary packed size:  " << encSize << " bytes (" <<
                        ((100 * encSize) / fileSize) << "% of the original)" << endl;
            }

            // Encode in printable characters...
            // FIXME!

            if (verbose)
            {
                cerr << "Latin1 encoded size: " << encSize << " bytes (" <<
                        ((100 * encSize) / fileSize) << "% of the original)" << endl;
            }

            // Create self extracting module
            if (sfx)
            {
                // FIXME
            }

            if (verbose)
            {
                cerr << "Final result:        " << encSize << " bytes (" <<
                        ((100 * encSize) / fileSize) << "% of the original)" << endl;
            }

            // Write the result...
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

