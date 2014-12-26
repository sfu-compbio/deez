#ifndef Compress_H
#define Compress_H

#include "Common.h"
#include "Stats.h"
#include "Parsers/BAMParser.h"
#include "Parsers/SAMParser.h"
#include "Fields/Sequence.h"
#include "Fields/ReadName.h"
#include "Fields/ReadNameLossy.h"
#include "Fields/MappingFlag.h"
#include "Fields/EditOperation.h"
#include "Fields/MappingQuality.h"
#include "Fields/QualityScore.h"
#include "Fields/PairedEnd.h"
#include "Fields/OptionalField.h"

class FileCompressor {
	vector<Parser*> parsers;
	vector<SequenceCompressor*> sequence;
	vector<EditOperationCompressor*> editOp;
	vector<Compressor*> readName;
	vector<MappingFlagCompressor*> mapFlag;
	vector<MappingQualityCompressor*> mapQual;
	vector<QualityScoreCompressor*> quality;
	vector<PairedEndCompressor*> pairedEnd;
	vector<OptionalFieldCompressor*> optField;

	FILE *outputFile;
	FILE *indexTmp;
	gzFile indexFile;

	size_t blockSize;

public:
	FileCompressor (const std::string &outFile, const std::vector<std::string> &samFiles, const std::string &genomeFile, int blockSize);
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

	static void compressBlock (Compressor *c, Array<uint8_t> &out, Array<uint8_t> &idxOut, size_t count);
	void outputBlock (Array<uint8_t> &out, Array<uint8_t> &idxOut);

public:
	void compress (void);
};

#endif // Compress_H
