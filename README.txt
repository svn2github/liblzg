liblzg v1.0
===========

About
-----

liblzg is a minimal implementation of an LZ77 class compression library. The
main characteristic of the library is that the decoding routine is very simple,
fast and requires little memory.

In general, liblzg does not compress as well as zlib, for instance, and the
encoder is usually slower than comparable compression routines. On the other
hand the decoder is very simple and very fast - ideal for systems with tight
memory limits or processing capabilities.

For more information, see the Doxygen documentation (in the doc folder).


Compiling
---------

The easisest way to compile liblzg and its tools is to use the GCC toolchain,
and use the Makefile in the src folder. Just type 'make' in the src folder,
which should create a static link library (in the src/lib folder) and the
example tools (in the src/tools folder).


License
-------

This project, and its source code, is released under the zlib/libpng license
(see LICENSE.txt). This means that it can be used in any product, commercial
or not.


Version history
---------------

v1.0 - 2010.10.28
- First release

