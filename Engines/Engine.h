#ifndef Engine_H
#define Engine_H

#include <vector>
#include "../Common.h"
#include "../Streams/Stream.h"

class Compressor {
protected:
	CompressionStream *stream;

public:
	FILE *debugStream;
	size_t compressedCount;

public:
	Compressor () { debugStream = 0; compressedCount = 0; }
	virtual ~Compressor (void) { if (debugStream) fclose(debugStream); }

public:
	// Resets out
	// set out size to compressed size block
	virtual void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {}
};

class Decompressor {
protected:
	DecompressionStream *stream;

public:
	virtual ~Decompressor (void) {}

public:
	virtual bool hasRecord (void) = 0;
	virtual void importRecords (uint8_t *in, size_t in_size) {} 
};

#endif
