// -*- mode: javascript; tab-width: 2; indent-tabs-mode: nil; -*-

//------------------------------------------------------------------------------
// This file is part of liblzg.
//
// Copyright (c) 2010 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Description
// -----------
// This is a JavaScript implementation of the LZG decoder.
//
// Example usage:
//
//   lzg = new lzgmini();
//   str = lzg.decode(compressedStr);
//
//------------------------------------------------------------------------------

function lzgmini() {

  // Constants
  this.LZG_HEADER_SIZE = 16;
  this.LZG_METHOD_COPY = 0;
  this.LZG_METHOD_LZG1 = 1;

  // LUT for decoding the copy length parameter
  this.LENGTH_DECODE_LUT = [2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
                            20,21,22,23,24,25,26,27,28,29,35,48,72,128];

  // Calculate the checksum
  this.calc_checksum = function(data) {
    var a = 1;
    var b = 0;
    var i = this.LZG_HEADER_SIZE;
    while (i < data.length)
    {
      a = (a + (data.charCodeAt(i) & 0xff)) & 0xffff;
      b = (b + a) & 0xffff;
      i++;
    }
    return b * 65536 + a;
  }

  // Decode LZG coded data
  this.decode = function(data) {
    // FIXME
  }
}

