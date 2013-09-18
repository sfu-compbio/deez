#ifndef MappingLocation_H
#define MappingLocation_H

#include "../Common.h"
#include "../Streams/Stream.h"
#include "../Engines/Engine.h"
#include "../Engines/GenericEngine.h"

class MappingLocationCompressor: public Compressor {
	CompressionStream *stitchStream;

	Array<uint8_t>	 records;
	Array<uint32_t>	 corrections;
	
    uint32_t lastLoc;
    std::string lastRef;

public:
	MappingLocationCompressor (int blockSize);
	~MappingLocationCompressor (void);

public:
	void addRecord (uint32_t tmpLoc, const std::string &tmpRef);
	void outputRecords (Array<uint8_t> &output);
};

class MappingLocationDecompressor: public Decompressor {
	DecompressionStream	*stitchStream;

	Array<uint8_t>	 records;
	Array<uint32_t>	 corrections;

	uint32_t lastLoc;
	
	int recordCount;
	int correctionCount;

public:
	MappingLocationDecompressor (int blockSize);
	~MappingLocationDecompressor (void);

private:
	void getNextCorrection (void);

public:
	uint32_t getRecord (void);
	bool hasRecord (void);
	void importRecords (uint8_t *in, size_t in_size);
};

#endif