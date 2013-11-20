#ifndef Decompress_H
#define Decompress_H

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

class FileDecompressor {
	Decompressor *decompressor[8];

	FILE *samFile;
	FILE *inFile;
	
	size_t inFileSz;
    size_t blockSize;

public:
	FileDecompressor (const std::string &inFile, const std::string &outFile, const std::string &genomeFile, int bs);
	~FileDecompressor (void);

private:
	void getMagic (void);
	void getComment (bool output);
	size_t getBlock (const std::string &chromosome, size_t start, size_t end);

public:
	void decompress (void);
	void decompress (const std::string &idxFilePath, const std::string &range);
};

#endif // Decompress_H