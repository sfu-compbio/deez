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

public:
	Compressor () { debugStream = 0; }
	virtual ~Compressor (void) { if (debugStream) fclose(debugStream); }

public:
	// Resets out
	// set out size to compressed size block
	virtual void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) = 0;
	virtual void getIndexData (Array<uint8_t> &out) = 0;
	virtual size_t compressedSize(void) { return stream->getCount(); }
};


template<typename T>
void compressArray (CompressionStream *c, Array<T> &in, Array<uint8_t> &out, size_t &outOffset) {
	size_t s = 0;
	if (in.size()) s = c->compress((uint8_t*)in.data(), 
		in.size() * sizeof(T), out, outOffset + 2 * sizeof(size_t));
	*(size_t*)(out.data() + outOffset) = s;
	*(size_t*)(out.data() + outOffset + sizeof(size_t)) = in.size() * sizeof(T);
	outOffset += s + 2 * sizeof(size_t);		
	out.resize(outOffset);
}

class Decompressor {
protected:
	DecompressionStream *stream;

public:
	virtual ~Decompressor (void) {}

public:
	virtual bool hasRecord (void) = 0;
	virtual void importRecords (uint8_t *in, size_t in_size) = 0;
	virtual void setIndexData (uint8_t *, size_t) = 0;
};

size_t decompressArray (DecompressionStream *d, uint8_t* &in, Array<uint8_t> &out);

#endif
