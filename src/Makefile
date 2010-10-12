# -*- mode: Makefile; tab-width: 4; indent-tabs-mode: t; -*-

################################################################################
# This file is part of liblzg.
#
# Copyright (c) 2010 Marcus Geelnard
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would
#    be appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not
#    be misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source
#    distribution.
################################################################################

# Compiler and linker settings
CC = gcc
CFLAGS = -c -O3 -funroll-loops -W -Wall
LC = gcc
LFLAGS = -L.
LIBS = -llzg
AR = ar
ARFLAGS = -rc
RM = rm -f

# Files
STATIC_LIB = liblzg.a
LZG = lzg
UNLZG = unlzg
OBJS = encode.o \
       decode.o \
       checksum.o
LZG_OBJS = lzg.o
UNLZG_OBJS = unlzg.o

# Master rule
all: $(STATIC_LIB) $(LZG) $(UNLZG)

# Clean rule
clean:
	$(RM) $(STATIC_LIB) $(OBJS) $(LZG) $(LZG_OBJS) $(UNLZG) $(UNLZG_OBJS)

# Library build rule
$(STATIC_LIB): $(OBJS)
	$(AR) $(ARFLAGS) -o $@ $(OBJS)

# Program build rules
$(LZG): $(LZG_OBJS) $(STATIC_LIB) 
	$(LC) $(LFLAGS) -o $@ $(LZG_OBJS) $(LIBS)

$(UNLZG): $(UNLZG_OBJS) $(STATIC_LIB) 
	$(LC) $(LFLAGS) -o $@ $(UNLZG_OBJS) $(LIBS)

# Object files build rules
encode.o: encode.c internal.h lzg.h
	$(CC) $(CFLAGS) $<

decode.o: decode.c internal.h lzg.h
	$(CC) $(CFLAGS) $<

checksum.o: checksum.c
	$(CC) $(CFLAGS) $<

lzg.o: lzg.c lzg.h
	$(CC) $(CFLAGS) $<

unlzg.o: unlzg.c lzg.h
	$(CC) $(CFLAGS) $<


