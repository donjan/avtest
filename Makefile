CC = g++
DEFNS = -D__STDC_CONSTANT_MACROS
LIBDIR = -L/usr/include/libavformat
LIBS = -lavformat -lavcodec -lavutil -lswscale -lz -lm
#~ LIBS = --libs libavformat libavcodec libswscale libavutil 

FFMPEG_LIBS = $(shell pkg-config libavformat libavcodec libavutil libswscale --libs)
FFMPEG_CFLAGS = $(shell pkg-config libavformat libavcodec libavutil libswscale --cflags)



#~ $(CC) $(OPTS) $(LIBDIR) -O2 getframe.cpp -o getframe $(LIBS)
#~ $(CC) $(OPTS) $(LIBDIR) -O2 $(FFMPEG_DEFS) $(FFMPEG_CFLAGS) -o getframe getframe.cpp $(LIBS)



all:
	g++ $(DEFNS) -o getframe getframe.cpp -L/usr/include/libavformat -lavformat
