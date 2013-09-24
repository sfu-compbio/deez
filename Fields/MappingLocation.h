#ifndef MappingLocation_H
#define MappingLocation_H

#include <string>

#include "../Common.h"
#include "../Engines/GenericEngine.h"
#include "../Streams/Order0Stream.h"
#include "../Streams/GzipStream.h"

class MappingLocationCompressor: 
	public GenericCompressor<size_t, AC0CompressionStream> 
{
	CompressionStream *stitchStream;
    uint32_t lastLoc;

public:
	MappingLocationCompressor (int blockSize);
	~MappingLocationCompressor (void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
};

class MappingLocationDecompressor: 
	public GenericDecompressor<size_t, AC0DecompressionStream> 
{
	DecompressionStream	*stitchStream;
	uint32_t lastLoc;

public:
	MappingLocationDecompressor (int blockSize);
	~MappingLocationDecompressor (void);

public:
	void importRecords (uint8_t *in, size_t in_size);
};

#endif