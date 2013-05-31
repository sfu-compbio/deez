#ifndef ReadName_H
#define ReadName_H

#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

class ReadNameCompressor: public Compressor {
	gzFile file;
	std::string buffer;

public:
	ReadNameCompressor (const std::string &filename, int blockSize);
	~ReadNameCompressor (void);

public:
	void addRecord (const std::string &rec);
	void outputRecords (void);
};

class ReadNameDecompressor: public Decompressor {
	gzFile file;
	std::vector<std::string> records;

	int recordCount;
	int blockSize;

public:
	ReadNameDecompressor (const std::string &filename, int bs);
	~ReadNameDecompressor (void);

public:
	std::string getRecord (void);
	void importRecords (void);
};

#endif
