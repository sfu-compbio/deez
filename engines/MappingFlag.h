#ifndef MappingFlag_H
#define MappingFlag_H

#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

class MappingFlagCompressor: public Compressor {
	gzFile file;
	std::vector<short> records;

public:
	MappingFlagCompressor (const std::string &filename, int blockSize);
	~MappingFlagCompressor (void);

public:
	void addRecord (short rec);
	void outputRecords (void);
};

class MappingFlagDecompressor: public Decompressor {
	gzFile file;
	std::vector<short> records;

	int recordCount;
	int blockSize;

public:
	MappingFlagDecompressor (const std::string &filename, int bs);
	~MappingFlagDecompressor (void);

public:
	short getRecord (void);
	void importRecords (void);
};

#endif
