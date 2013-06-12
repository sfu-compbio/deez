CC      		= g++
DEBUG			= -O3
CFLAGS   	= -O3 -fno-omit-frame-pointer -c -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
LDFLAGS  	= -lz
SOURCES 	  := $(shell find $(SOURCEDIR) -name '*.cc')
OBJECTS  	= $(SOURCES:.cc=.o)
EXECUTABLE 	= dz

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f Engines/*.o Parsers/*.o Streams/*.o Fields/*.o *.o dz

