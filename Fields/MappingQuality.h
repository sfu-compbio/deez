#ifndef MappingQuality_H
#define MappingQuality_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

class MappingQualityCompressor: public GenericCompressor<uint8_t, GzipCompressionStream<6> > {
	virtual const char *getID () const { return "MappingQuality"; }

public:
	MappingQualityCompressor(int blockSize):
		GenericCompressor<uint8_t, GzipCompressionStream<6> >(blockSize) {}
};

class MappingQualityDecompressor: public GenericDecompressor<uint8_t, GzipDecompressionStream> {
	virtual const char *getID () const { return "MappingQuality"; }

public:
	MappingQualityDecompressor(int blockSize):
		GenericDecompressor<uint8_t, GzipDecompressionStream>(blockSize) {}
};

#endif
