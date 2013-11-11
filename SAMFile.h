#ifndef SAMFile_H
#define SAMFile_H

#include "Common.h"
#include "Parsers/BAMParser.h"
#include "Parsers/SAMParser.h"
#include "Fields/Sequence.h"
#include "Fields/ReadName.h"
#include "Fields/MappingFlag.h"
#include "Fields/MappingLocation.h"
#include "Fields/MappingQuality.h"
#include "Fields/QualityScore.h"
#include "Fields/PairedEnd.h"
#include "Fields/OptionalField.h"

class SAMFileCompressor {
	Parser *parser;
	Compressor *compressor[8];

	FILE *outputFile;
	gzFile indexFile;

	size_t blockSize;

public:
	SAMFileCompressor (const std::string &outFile, const std::string &samFile, const std::string &genomeFile, int blockSize);
	~SAMFileCompressor (void);

private:
	/* Structure:
	 	* magic[4]
	 	* comment[..\0]
		* chrEqual[1]
		* chrEqual=1? chr[..\0]
		* blockSz[8]
		* blockSz>0?  block[..blockSz] */
	void outputMagic(void);
	void outputComment(void);
	void outputRecords(void);

public:
	void compress (void);
};

class SAMFileDecompressor {
	Decompressor *decompressor[8];

	FILE *samFile;
	FILE *inFile;
	
	size_t inFileSz;
    size_t blockSize;

public:
	SAMFileDecompressor (const std::string &inFile, const std::string &outFile, const std::string &genomeFile, int bs);
	~SAMFileDecompressor (void);

private:
	void getMagic (void);
	void getComment (bool output);
	size_t getBlock (const std::string &chromosome, size_t start, size_t end);

public:
	void decompress (void);
	void decompress (const std::string &idxFilePath, const std::string &range);
};

#endif
