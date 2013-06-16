#ifndef MappingFlag_H
#define MappingFlag_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

class MappingFlagCompressor: public GenericCompressor<short, GzipCompressionStream<6> > {
	virtual const char *getID () const { return "MappingFlag"; }

public:
	MappingFlagCompressor(int blockSize):
		GenericCompressor<short, GzipCompressionStream<6> >(blockSize) {}
};

class MappingFlagDecompressor: public GenericDecompressor<short, GzipDecompressionStream> {
	virtual const char *getID () const { return "MappingFlag"; }

public:
	MappingFlagDecompressor(int blockSize):
		GenericDecompressor<short, GzipDecompressionStream>(blockSize) {}
};

#endif
