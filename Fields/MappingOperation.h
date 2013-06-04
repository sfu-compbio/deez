#ifndef MappingOperation_H
#define MappingOperation_H

#include "../Common.h"
#include "../Streams/Stream.h"
#include "../Engines/Engine.h"
#include "../Engines/GenericEngine.h"

struct Locs {
	uint32_t loc;
	short ref;
};

struct Tuple {
	uint32_t ref;
	uint32_t first;
	uint32_t second;
};

class MappingOperationCompressor: public Compressor {
	CompressionStream *stitchStream;

	std::vector<uint8_t>	 records;
	std::vector<Tuple>		 corrections;
	
    uint32_t    lastLoc;
	short		lastRef;

public:
	MappingOperationCompressor (int blockSize);
	~MappingOperationCompressor (void);

public:
	void addRecord (uint32_t tmpLoc, short tmpRef);
	void outputRecords (std::vector<char> &output);
};

class MappingOperationDecompressor: public Decompressor {
	DecompressionStream	*stitchStream;

	std::vector<Tuple> 	     corrections;
	std::vector<Locs> 	     records;

	uint32_t    lastLoc;
	short		lastRef;
	Tuple 	    lastCorrection;

	int recordCount;
	int correctionCount;

public:
	MappingOperationDecompressor (int blockSize);
	~MappingOperationDecompressor (void);

private:
	void getNextCorrection (void);

public:
	Locs getRecord (void);
	bool hasRecord (void);
	void importRecords (const std::vector<char> &input);
};

#endif
