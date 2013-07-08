CC      		= g++
DEBUG			= 
CFLAGS   	= -O3 -g -fno-omit-frame-pointer -c -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
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
	find -name '*.o' -delete
	rm -rf dz

