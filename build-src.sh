#!/bin/bash

# Name of the distribution
distname=liblzg-1.0.0

# Clean the source directories
echo Cleaning up...
cd src && make -f Makefile clean && cd ..

# Build all the necessary files
echo Building documentation...
cd doc && make -f Makefile clean && make -f Makefile clean && cd ..

# Set up a temporary directory
tmproot=/tmp/liblzg-$USER-$$
mkdir $tmproot
tmpdir=$tmproot/$distname
mkdir $tmpdir

# Copy files
echo Copying files to $tmpdir...
cp *.txt $tmpdir/
mkdir $tmpdir/src
cp src/Makefile* $tmpdir/src/
mkdir $tmpdir/src/include
cp src/include/*.h $tmpdir/src/include/
mkdir $tmpdir/src/lib
cp src/lib/*.c src/lib/*.h src/lib/Makefile* $tmpdir/src/lib/
mkdir $tmpdir/src/tools
cp src/tools/*.c src/tools/*.h src/tools/Makefile* $tmpdir/src/tools/

mkdir $tmpdir/doc
cp doc/*.svg $tmpdir/doc/
mkdir $tmpdir/doc/APIReference
cp doc/APIReference/* $tmpdir/doc/APIReference/

# Create archives
olddir=`pwd`
cd $tmproot
tar -cvf $distname-src.tar $distname
bzip2 -9 $distname-src.tar
zip -9r $distname-src.zip $distname
cd $olddir
cp $tmproot/*.bz2 $tmproot/*.zip ./

# Remove temporary directory
rm -rf $tmproot

