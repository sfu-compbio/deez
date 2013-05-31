#ifndef MappingQuality_H
#define MappingQuality_H

#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

class MappingQualityCompressor: public Compressor {
	gzFile file;
	std::vector<uint8_t> records;

public:
	MappingQualityCompressor (const std::string &filename, int blockSize);
	~MappingQualityCompressor (void);

public:
	void addRecord (unsigned char mq);
	void outputRecords (void);
};

class MappingQualityDecompressor: public Decompressor {
	gzFile file;
	std::vector<uint8_t> records;

	int recordCount;
	int blockSize;

public:
	MappingQualityDecompressor (const std::string &filename, int bs);
	~MappingQualityDecompressor (void);

public:
	unsigned char getRecord (void);
	void importRecords (void);
};

#endif
