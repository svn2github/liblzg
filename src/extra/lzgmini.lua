-- -*- mode: lua; tab-width: 2; indent-tabs-mode: nil; -*-

-- This file is part of liblzg.
--
-- Copyright (c) 2010 Marcus Geelnard
--
-- This software is provided 'as-is', without any express or implied
-- warranty. In no event will the authors be held liable for any damages
-- arising from the use of this software.
--
-- Permission is granted to anyone to use this software for any purpose,
-- including commercial applications, and to alter it and redistribute it
-- freely, subject to the following restrictions:
--
-- 1. The origin of this software must not be misrepresented; you must not
--    claim that you wrote the original software. If you use this software
--    in a product, an acknowledgment in the product documentation would
--    be appreciated but is not required.
--
-- 2. Altered source versions must be plainly marked as such, and must not
--    be misrepresented as being the original software.
--
-- 3. This notice may not be removed or altered from any source
--    distribution.

-- Calculate the checksum
function LZG_CalcChecksum(data)
  local a = 1
  local b = 0
  local i
  for i = 17,data:len() do
    a = (a + data:byte(i)) % 65536
    b = (b + a) % 65536
  end
  return b * 65536 + a
end

-- Decode LZG coded data
function LZG_Decode(data)
  -- Check magic ID
  if (data:len() < 16) or (data:byte(1) ~= 76) or
     (data:byte(2) ~= 90) or (data:byte(3) ~= 71) then
    return ''
  end

  -- Check the checksum
  local checksum = data:byte(12) * 16777216 + data:byte(13) * 65536 +
                   data:byte(14) * 256 + data:byte(15)
  if LZG_CalcChecksum(data) ~= checksum then
    return ''
  end

  -- Check which method to use (LZG_METHOD_LZG1 = 1)
  local method = data:byte(16)
  if method == 1 then
    -- LUT for decoding the copy length parameter
    local LZG_LENGTH_DECODE_LUT = {3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
                                   18,19,20,21,22,23,24,25,26,27,28,29,35,48,72,128}

    -- Get marker symbols
    local m1 = data:byte(17)
    local m2 = data:byte(18)
    local m3 = data:byte(19)
    local m4 = data:byte(20)

    -- Main decompression loop
    local dst = ''
    local symbol, b, b2, b3, length, offset, copy, i
    local k = 21
    while k < data:len() do
      symbol = data:byte(k); k = k + 1
      if (symbol ~= m1) and (symbol ~= m2) and (symbol ~= m3) and (symbol ~= m4) then
        -- Literal copy
        dst = dst..string.char(symbol)
      else
        b = data:byte(k); k = k + 1
        if b ~= 0 then
          if symbol == m1 then
            -- marker1 - "Distant copy"
            length = LZG_LENGTH_DECODE_LUT[b % 32];
            b2 = data:byte(k); k = k + 1
            b3 = data:byte(k); k = k + 1
            offset = math.floor(b / 32) * 65536 + b2 * 256 + b3 + 2056
          elseif symbol == m2 then
            -- marker2 - "Medium copy"
            length = LZG_LENGTH_DECODE_LUT[b % 32];
            b2 = data:byte(k); k = k + 1
            offset = math.floor(b / 32) * 256 + b2 + 8
          elseif symbol == m3 then
            -- marker3 - "Short copy"
            length = math.floor(b / 64) + 3
            offset = (b % 64) + 8
          else
            -- marker4 - "Near copy (incl. RLE)"
            length = LZG_LENGTH_DECODE_LUT[b % 32];
            offset = math.floor(b/32) + 1
          end

          -- Copy the corresponding data from the history window
          for i=1,length do
            dst = dst..string.char(dst:byte(dst:len()+1-offset))
          end
        else
          -- Literal copy (single occurance of a marker symbol)
          dst = dst..string.char(symbol)
        end
      end
    end
    return dst
  elseif method == 0 then
    -- Plain copy (LZG_METHOD_COPY)
    return data:sub(17)
  else
    -- Unknown method
    return ''
  end
end

