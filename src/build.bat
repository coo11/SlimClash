@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b %errorlevel%

python script\version.py
if errorlevel 1 exit /b %errorlevel%

windres -Isrc src\launcher.rc -O coff -o src\launcher-res.o
if errorlevel 1 exit /b %errorlevel%

gcc -std=c11 -O2 -g0 -Wall -Wextra -municode -mwindows src\launcher.c src\launcher-res.o -o SlimClash.exe -lshell32 -ladvapi32 -lwinhttp
if errorlevel 1 exit /b %errorlevel%

strip --strip-debug SlimClash.exe
if errorlevel 1 exit /b %errorlevel%

echo Built SlimClash.exe
