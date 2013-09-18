CC ?= g++
CFLAGS = -fno-omit-frame-pointer -c -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
LDFLAGS = -lz -lpthread

SOURCES := $(shell find $(SOURCEDIR) -name '*.cc' -not -path "./run/*" -not -path "./Point*/*")
OBJECTS = $(SOURCES:.cc=.o)
EXECUTABLE = dz

all: CFLAGS += -O3 -DNDEBUG
all: $(SOURCES) $(EXECUTABLE)

debug: CFLAGS += -g
debug: $(SOURCES) $(EXECUTABLE)
 
profile: LDFLAGS += -ltcmalloc -lprofiler
profile: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	find -name '*.o' -delete
	rm -rf dz
