#!/bin/sh
make

echo "Stripping binary"
mipsel-linux-strip gen_gcw0;

echo "Removing existing opk package"
rm -rf genplus.opk

echo "\nBuilding new opk package"
mksquashfs gen_gcw0 opk-data/* genplus.opk -all-root -noappend -no-exports -no-xattrs
