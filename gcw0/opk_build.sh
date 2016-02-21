#!/bin/sh

# Set debug to 1 if you want a debug build, 0 if you want a release build
debug=0
sdl2=1

if [ "$sdl2" = "1" ]
then
echo '\n WARNING!!! SDL2 implementation is currently experimental and incomplete. \n'
fi

echo '\nCompiling source code\n'
make DEBUG=$debug SDL2=$sdl2 -f Makefile.gcw

if [ "$debug" = "0" ]
then
	echo '\n\nStripping binary\n'
	mipsel-linux-strip gen_gcw0
fi

echo '\n\nRemoving existing opk package\n'
rm -rf genplus.opk

echo '\n\nBuilding new opk package\n'
mksquashfs gen_gcw0 opk-data/* genplus.opk -all-root -noappend -no-exports -no-xattrs

echo '\n\nDONE!\n'
