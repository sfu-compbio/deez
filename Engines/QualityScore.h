#ifndef QualityScore_H
#define QualityScore_H

#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

class QualityScoreCompressor: public Compressor {
	gzFile file;
	std::string buffer;

public:
	QualityScoreCompressor (const std::string &filename, int blockSize);
	~QualityScoreCompressor (void);

public:
	void addRecord (const std::string &rec);
	void outputRecords (void);
};

class QualityScoreDecompressor: public Decompressor {
	gzFile file;
	std::vector<std::string> records;

	int recordCount;
	int blockSize;

public:
	QualityScoreDecompressor (const std::string &filename, int bs);
	~QualityScoreDecompressor (void);

public:
	std::string getRecord (void);
	void importRecords (void);
};

#endif
