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
CFLAGS = -c -g -O3 -W -Wall -I../include
LC = gcc
LFLAGS = -L../lib
LIBS = -llzg
RM = rm -f

# Files
LZG = lzg
UNLZG = unlzg
LZG_OBJS = lzg.o
UNLZG_OBJS = unlzg.o
STATIC_LIB = ../lib/liblzg.a

.PHONY: all clean

# Master rule
all: $(LZG) $(UNLZG)

# Clean rule
clean:
	$(RM) $(LZG) $(LZG_OBJS) $(UNLZG) $(UNLZG_OBJS)

# Program build rules
$(LZG): $(LZG_OBJS) $(STATIC_LIB)
	$(LC) $(LFLAGS) -o $@ $(LZG_OBJS) $(LIBS)

$(UNLZG): $(UNLZG_OBJS) $(STATIC_LIB)
	$(LC) $(LFLAGS) -o $@ $(UNLZG_OBJS) $(LIBS)

# Object files build rules
lzg.o: lzg.c ../include/lzg.h
	$(CC) $(CFLAGS) $<

unlzg.o: unlzg.c ../include/lzg.h
	$(CC) $(CFLAGS) $<

