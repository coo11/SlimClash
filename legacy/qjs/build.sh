#!/bin/sh
# This a cross-platform build script for QuickJS DLL version under Ubuntu.
# QuickJS version: 2021-03-27
# Build failure  : The executable file compiled by MinGW-W64 (gcc version 9.3-win32 20200320) 
#                  under Ubuntu(Ubuntu 22.04 LTS) has a quite larger size than under Windows.
#                  It is likely because of the low version.
#                  So this script MUST run under Windows.

rm -rf dist_linux dist_win quickjs

git clone https://github.com/bellard/quickjs
cd quickjs

# Assuming that the GCC and MinGW-W64 has been installed.

# Compile to DLL
mingw32-make CONFIG_WIN32=y EXE=.dll QJS_OBJS=\$\(QJS_LIB_OBJS\) LDEXPORT="-shared -fPIC -static -s" EXTRA_LIBS=-lpthread qjs.dll

# Compile to QuickJS Compiler
mkdir ../dist_linux ../dist_win
# Windows
mingw32-make CONFIG_WIN32=y HOST_LIBS="-L./ -lqjs" QJS_LIB_OBJS= EXTRA_LIBS="-L./ -lqjs" qjs.exe qjsc.exe
cp qjs.dll qjs.exe qjsc.exe quickjs.h quickjs-libc.h ../pack.bat -t ../dist_win
mingw32-make clean
# Linux
#make LDEXPORT="-static -s" qjsc
#cp qjs.dll qjsc quickjs.h quickjs-libc.h ../pack.sh -t ../dist_linux
#make clean