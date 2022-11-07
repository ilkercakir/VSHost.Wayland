#!/bin/bash
gcc -fPIC -c Effect1.c $(pkg-config --cflags gtk+-3.0)
gcc -shared -fPIC -Wl,-soname,Effect1.so -o Effect1.so Effect1.o -lc

