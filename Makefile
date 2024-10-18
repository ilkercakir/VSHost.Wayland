CC=gcc
CFLAGS=-Wall -c -g -DUSE_OPENGL -DUSE_EGL -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -g -ftree-vectorize -pipe -Wno-psabi $(shell pkg-config --cflags gtk+-3.0) -Wno-deprecated-declarations
LDFLAGS=-Wall -o -g -Wl,--whole-archive -Wl,--no-whole-archive -rdynamic $(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --libs libavcodec libavformat libavutil libswscale libswresample) -lpthread -lrt -ldl -lm -lGLESv2 -lEGL $(shell pkg-config --libs gtk+-3.0) -lasound $(shell pkg-config --libs gtk+-3.0) $(shell pkg-config --libs sqlite3) -lpulse
SOURCES=VSHost.c VStudio.c VSJam.c VSEffect.c VSMessage.c AudioDev.c AudioMic.c AudioSpk.c AudioMixer.c AudioPipeNB.c AudioEncoder.c VSTMediaPlayer.c VSTVideoPlayer.c YUV420RGBgl.c VideoQueue.c PulseAudio.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=VSHost

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f *.o
	-rm -f $(EXECUTABLE)
