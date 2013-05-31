#ifndef OptionalField_H
#define OptionalField_H

#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

class OptionalFieldCompressor: public Compressor {
	gzFile file;
	std::string buffer;

    std::vector<short>               fieldIdx;
    std::vector<std::vector<string>> fieldData;
    int fieldCount;



public:
	OptionalFieldCompressor (const std::string &filename, int blockSize);
	~OptionalFieldCompressor (void);

public:
	void addRecord (const std::string &rec);
	void outputRecords (void);
};

class OptionalFieldDecompressor: public Decompressor {
	gzFile file;
	std::vector<std::string> records;

	int recordCount;
	int blockSize;

public:
	OptionalFieldDecompressor (const std::string &filename, int bs);
	~OptionalFieldDecompressor (void);

public:
	std::string getRecord (void);
	void importRecords (void);
};

#endif
