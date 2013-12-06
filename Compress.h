#ifndef Compress_H
#define Compress_H

#include "Common.h"
#include "Parsers/BAMParser.h"
#include "Parsers/SAMParser.h"
#include "Fields/Sequence.h"
#include "Fields/ReadName.h"
#include "Fields/MappingFlag.h"
#include "Fields/EditOperation.h"
#include "Fields/MappingQuality.h"
#include "Fields/QualityScore.h"
#include "Fields/PairedEnd.h"
#include "Fields/OptionalField.h"

class FileCompressor {
	Parser *parser;
	SequenceCompressor *sequence;
	EditOperationCompressor *editOp;
	ReadNameCompressor *readName;
	MappingFlagCompressor *mapFlag;
	MappingQualityCompressor *mapQual;
	QualityScoreCompressor *quality;
	PairedEndCompressor *pairedEnd;
	OptionalFieldCompressor *optField;

	FILE *outputFile;
	FILE *indexTmp;
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
	void outputBlock (Compressor *c, Array<uint8_t> &out, size_t count);

public:
	void compress (void);
};

#endif // Compress_H
