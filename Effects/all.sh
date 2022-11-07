#!/bin/bash
rm *.o
rm *.so

gcc -fPIC -c BiQuad.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c DelayS.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c HaasS.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c VFO.c $(pkg-config --cflags gtk+-3.0)

gcc -fPIC -c Gain.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Tremolo.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Delay.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Haas.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Modulation.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Equalizer.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c FoldingDistortion.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Overdrive.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Mono.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c ReverbSchroeder.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Chorus.c $(pkg-config --cflags gtk+-3.0)
gcc -fPIC -c Stereoizer.c $(pkg-config --cflags gtk+-3.0)

gcc -shared -fPIC -Wl,-soname,Gain.so -o Gain.so Gain.o -lc
gcc -shared -fPIC -Wl,-soname,Tremolo.so -o Tremolo.so Tremolo.o -lc
gcc -shared -fPIC -Wl,-soname,Delay.so -o Delay.so Delay.o DelayS.o -lc
gcc -shared -fPIC -Wl,-soname,Haas.so -o Haas.so Haas.o HaasS.o DelayS.o -lc
gcc -shared -fPIC -Wl,-soname,Modulation.so -o Modulation.so Modulation.o VFO.o -lc
gcc -shared -fPIC -Wl,-soname,Equalizer.so -o Equalizer.so Equalizer.o BiQuad.o -lc
gcc -shared -fPIC -Wl,-soname,FoldingDistortion.so -o FoldingDistortion.so FoldingDistortion.o -lc
gcc -shared -fPIC -Wl,-soname,Overdrive.so -o Overdrive.so Overdrive.o BiQuad.o -lc
gcc -shared -fPIC -Wl,-soname,Mono.so -o Mono.so Mono.o -lc
gcc -shared -fPIC -Wl,-soname,ReverbSchroeder.so -o ReverbSchroeder.so ReverbSchroeder.o BiQuad.o DelayS.o -lc
gcc -shared -fPIC -Wl,-soname,Chorus.so -o Chorus.so Chorus.o DelayS.o VFO.o -lc
gcc -shared -fPIC -Wl,-soname,Stereoizer.so -o Stereoizer.so Stereoizer.o HaasS.o DelayS.o VFO.o -lc