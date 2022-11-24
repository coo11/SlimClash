#!/bin/sh
# This a cross-platform build script for QuickJS DLL version under Ubuntu.
# QuickJS version: 2021-03-27

rm -rf dist_linux dist_win quickjs

git clone https://ghproxy.com/https://github.com/bellard/quickjs
cd quickjs

# Assuming that the GCC and MinGW-W64 has been installed.
# Under Windows prefix "mingw32-" should add to "make".

# Compile to DLL
make CONFIG_WIN32=y EXE=.dll QJS_OBJS=\$\(QJS_LIB_OBJS\) LDEXPORT="-shared -fPIC -static -s" EXTRA_LIBS=-lpthread qjs.dll
make clean
# Compile QuickJS Compiler
# Source code doesn't give a variable to insert argument '-s' to compile qjsc.
# So '-s' should add to $(LIBS) to reduce size.
mkdir ../dist_linux ../dist_win
# Windows
cp Makefile Makefile_bak
sed -i "s/^\(QJS_OBJS=.*\)\s*\$(QJS_LIB_OBJS)/\1/" Makefile
sed -i "s/^\(qjsc\$(EXE):.*\)\s*\$(QJS_LIB_OBJS)/\1/" Makefile
make CONFIG_WIN32=y EXTRA_LIBS="-s -L./ -lqjs" qjs.exe qjsc.exe
cp qjs.dll qjs.exe qjsc.exe quickjs.h quickjs-libc.h ../pack.bat -t ../dist_win
make clean
rm -f Makefile
mv Makefile_bak Makefile
#Linux
make LIBS="-s -lm -ldl -lpthread" qjsc
cp qjs.dll qjsc quickjs.h quickjs-libc.h ../pack.sh -t ../dist_linux
make clean