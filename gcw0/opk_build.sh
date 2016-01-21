#!/bin/sh

echo '\nCompiling source code\n'
make -f Makefile.gcw

echo '\n\nStripping binary\n'
mipsel-linux-strip gen_gcw0;

echo '\n\nRemoving existing opk package\n'
rm -rf genplus.opk

echo '\n\nBuilding new opk package\n'
mksquashfs gen_gcw0 opk-data/* genplus.opk -all-root -noappend -no-exports -no-xattrs

echo '\n\nDONE!\n'
