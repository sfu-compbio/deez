#ifndef Engine_H
#define Engine_H

#include <vector>
#include "../Common.h"
#include "../Streams/Stream.h"

class Compressor {
protected:
	CompressionStream *stream;

public:
	virtual ~Compressor (void) {}

public:
	virtual void outputRecords (std::vector<char> &output) = 0;
};

class Decompressor {
protected:
	DecompressionStream *stream;

public:
	virtual ~Decompressor (void) {}

public:
	virtual bool hasRecord (void) = 0;
	virtual void importRecords (const std::vector<char> &input) = 0;
};

#endif
