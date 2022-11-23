@ECHO OFF

CD /D %~dp0

qjsc.exe -e -o "%~n1.c" -flto -fno-eval "%1"

"C:\mingw64\bin\gcc.exe" -o "%~n1.exe" -I./ -L./ -lqjs "%~n1.c"

del "%~n1.c"