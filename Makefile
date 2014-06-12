CC = g++
CFLAGS = -c -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE  -std=c++0x
LDFLAGS = -lz -lpthread 

SOURCES := $(shell find $(SOURCEDIR) -name '*.cc' -not -path "./run/*")
OBJECTS = $(SOURCES:.cc=.o)
EXECUTABLE = deez
TESTEXE = deeztest

all: CFLAGS += -O3 -DNDEBUG
all: $(SOURCES) $(EXECUTABLE)

debug: CFLAGS += -g -fno-omit-frame-pointer
debug: $(SOURCES) $(EXECUTABLE)

superdebug: CFLAGS += -g -O0 -fno-inline
superdebug: $(SOURCES) $(EXECUTABLE)

profile: CFLAGS += -g -pg -O3
profile: LDFLAGS += -pg
profile: $(SOURCES) $(EXECUTABLE)

gprofile: LDFLAGS += -ltcmalloc -lprofiler
gprofile: CFLAGS += -g
gprofile: $(SOURCES) $(EXECUTABLE)

test: CFLAGS += -O3 -DNDEBUG -std=c++11
test: $(SOURCES) $(TESTEXE)

testdebug: CFLAGS += -g -std=c++11
testdebug: $(SOURCES) $(TESTEXE)

$(TESTEXE): OBJECTS := $(subst ./Main.o,,$(OBJECTS))
$(TESTEXE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(EXECUTABLE): OBJECTS := $(subst ./Test.o,,$(OBJECTS))
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	find -name '*.o' -delete
	rm -rf $(EXECUTABLE) $(TESTEXE)
