#ifndef ReadName_H
#define ReadName_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"

class ReadNameCompressor: public StringCompressor<GzipCompressionStream<6> > {
	virtual const char *getID () const { return "ReadName"; }

public:
	ReadNameCompressor(int blockSize):
		StringCompressor<GzipCompressionStream<6> >(blockSize) {}
};

class ReadNameDecompressor: public StringDecompressor<GzipDecompressionStream> {
	virtual const char *getID () const { return "ReadName"; }

public:
	ReadNameDecompressor(int blockSize):
		StringDecompressor<GzipDecompressionStream>(blockSize) {}
};

#endif
