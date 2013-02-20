CC = g++
DEFNS = -D__STDC_CONSTANT_MACROS
#LIBS = -lavformat -lavcodec -lavutil -lswscale -lz -lm
LIBS = -lavformat

all:
	$(CC) $(DEFNS) -o getframe getframe.cpp -O2 $(LIBS)
