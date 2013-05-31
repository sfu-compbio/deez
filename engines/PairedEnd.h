#ifndef PairedEnd_H
#define PairedEnd_H

#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

class PairedEndCompressor: public Compressor {
	gzFile file;
	std::vector<int> records;

public:
	PairedEndCompressor (const std::string &filename, int blockSize);
	~PairedEndCompressor (void);

public:
	void addRecord (int chr, int pos, int tlen);
	void outputRecords (void);
};

class PairedEndDecompressor: public Decompressor {
	gzFile file;
	std::vector<int> records;

	int recordCount;
	int blockSize;

public:
	PairedEndDecompressor (const std::string &filename, int bs);
	~PairedEndDecompressor (void);

public:
	int getRecord (void);
	void importRecords (void);
};

#endif

