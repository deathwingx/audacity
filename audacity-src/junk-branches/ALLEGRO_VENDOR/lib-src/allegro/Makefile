# Make allegro library

CCC = g++
CC = gcc

CFLAGS = -g -O
OBJ = \
	allegro.cpp.o \
	allegrord.cpp.o \
	allegrowr.cpp.o \
	mfallegro.cpp.o \
	mfmidi.cpp.o \
	strparse.cpp.o \
	stimelinux.c.o 

allegro.a: $(OBJ)
	ar ruv allegro.a $(OBJ)
	ranlib allegro.a

clean:
	rm -f allegro.a *.o

%.cpp.o: %.cpp Makefile
	$(CCC) -c $(CFLAGS) $< -o $@

%.c.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@
