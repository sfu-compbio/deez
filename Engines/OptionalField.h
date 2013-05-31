#ifndef OptionalField_H
#define OptionalField_H

#include <deque>
#include <vector>
#include <string>
#include <zlib.h>

#include "../Common.h"
#include "Compressor.h"
#include "Decompressor.h"

class OptionalFieldCompressor: public Compressor {
	gzFile keyFile;
	gzFile valueFile;

	 std::vector<short> fieldIdx;		// indices for tags
	 std::vector<std::string> fieldKey;		// tags
	 std::vector<char> fieldType;    // types
    std::vector<std::vector<std::string> > fieldData;		// tag contents
	 std::vector<std::vector<short> > records; 		// tag list for each record


public:
	OptionalFieldCompressor (const std::string &filename, int blockSize);
	~OptionalFieldCompressor (void);

public:
	void addRecord (const std::string &rec);
	void outputRecords (void);
};

class OptionalFieldDecompressor: public Decompressor {
	gzFile keyFile;
	gzFile valueFile;

	std::vector<std::string> fieldKey;	// tags
	std::vector<char> fieldType;  // types
   std::vector<std::deque<std::string> > fieldData;	// tag contents
	std::vector<std::vector<short> > records; 	// tag list for each record

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
