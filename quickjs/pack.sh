fname=$(basename -- "$1")
fext="${fname%.*}"
fname="${fname%.*}"

./qjsc -e -o "$fname.c" -flto -fno-eval "$1"

x86_64-w64-mingw32-gcc -s -o "$fname.exe" -I./ -L./ -lqjs "$fname.c"

rm -f "$fname.c"