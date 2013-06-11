CC=g++
CFLAGS=-O3 -c -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
LDFLAGS=-lz #-ltcmalloc -lprofiler
SOURCES=$(wildcard ./Engines/*.cc) $(wildcard ./Fields/*.cc) $(wildcard ./Parsers/*.cc) $(wildcard ./Streams/*.cc) $(wildcard ./*.cc)
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=dz

all: $(SOURCES) $(EXECUTABLE) 

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f Engines/*.o Parsers/*.o Streams/*.o Fields/*.o *.o dz

