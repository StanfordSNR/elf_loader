#!/bin/sh -e

echo 'Compile '$1'.c into '$1'.wasm';
clang -Os --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--export-all -o $1.wasm $1.c 

echo 'wasm2c '$1'.wasm to '$1'-new.c';
wasm2c -o $1-new.c $1.wasm

echo 'build '$1'-new.c to '$1'-new.o';
cp ../../wasm-rt/wasm-rt.h wasm-rt.h
gcc -c $1-new.c 
