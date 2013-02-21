.PHONY: getframe sample

CC = g++
DEFNS = -D__STDC_CONSTANT_MACROS
#~ LIBS = -lavformat -lavcodec -lavutil -lswscale -lz -lm
LIBS = -lavformat

all: getframe

getframe:
	$(CC) $(DEFNS) -o getframe getframe.cpp -O2 $(LIBS)

sample:
	$(CC) $(DEFNS) -o sample avcodec_sample.cpp -O2 $(LIBS)

