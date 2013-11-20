#ifndef Compress_H
#define Compress_H

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

class FileCompressor {
	Parser *parser;
	Compressor *compressor[8];

	FILE *outputFile;
	gzFile indexFile;

	size_t blockSize;

public:
	FileCompressor (const std::string &outFile, const std::string &samFile, const std::string &genomeFile, int blockSize);
	~FileCompressor (void);

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

#endif // Compress_H
