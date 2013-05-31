#ifndef MappingOperation_H
#define MappingOperation_H

#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

struct Locs {
	std::string ref;
	uint32_t    loc;
};

struct Tuple {
	uint32_t ref;
	uint32_t first;
	uint32_t second;
};

class MappingOperationCompressor: public Compressor {
	gzFile lociFile;
	gzFile metadataFile;
	gzFile stitchFile;

	std::vector<uint8_t>      records;
	std::vector<Tuple>        corrections;
	std::vector<std::string>  references;
	std::vector<Locs> 	      drecords;

    uint32_t    lastLoc;
	std::string lastRef;

public:
	MappingOperationCompressor (const std::string &filename, int blockSize);
	~MappingOperationCompressor (void);

public:
	void addMetadata (const std::string &ref);
	void addRecord (uint32_t tmpLoc, const std::string &tmpRef);
	void outputRecords (void);
};

class MappingOperationDecompressor: public Compressor {
	gzFile lociFile;
	gzFile metadataFile;
	gzFile stitchFile;

	std::vector<uint8_t>     records;
	std::vector<Tuple> 	     corrections;
	std::vector<std::string> references;
	std::vector<Locs> 	     drecords;

	uint32_t    lastLoc;
	std::string lastRef;
	Tuple 	    lastCorrection;

	int recordCount;
	int blockSize;

public:
	MappingOperationDecompressor (const std::string &filename, int blockSize);
	~MappingOperationDecompressor (void);

private:
	void getNextCorrection (void);

public:
	void getMetaData (void);
	Locs getRecord (void);
	void importRecords (void);
};

#endif
