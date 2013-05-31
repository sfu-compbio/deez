#ifndef EditOperation_H
#define EditOperation_H

#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

class EditOperationCompressor: public Compressor {
	gzFile file;
	std::string buffer;

public:
	EditOperationCompressor (const std::string &filename, int blockSize);
	~EditOperationCompressor (void);

public:
	void addRecord (const std::string &rec);
	void outputRecords (void);
};


class EditOperationDecompressor: public Decompressor {
	gzFile file;
	std::vector<std::string> records;

	int recordCount;
	int blockSize;

public:
	EditOperationDecompressor (const std::string &filename, int bs);
	~EditOperationDecompressor (void);

public:
	std::string getRecord (void);
	void importRecords (void);
};

#endif
