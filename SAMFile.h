#ifndef SAMFile_H
#define SAMFile_H

#include "Common.h"
#include "Parsers/SAMParser.h"
#include "Fields/Reference.h"
#include "Fields/ReadName.h"
#include "Fields/MappingFlag.h"
#include "Fields/MappingOperation.h"
#include "Fields/MappingQuality.h"
#include "Fields/EditOperation.h"
#include "Fields/QualityScore.h"
#include "Fields/PairedEnd.h"
#include "Fields/OptionalField.h"

class SAMFileCompressor {
	SAMParser parser;

	ReferenceCompressor 	   reference;
	ReadNameCompressor 		   readName;
	MappingFlagCompressor 	   mappingFlag;
	MappingOperationCompressor mappingOperation;
    MappingQualityCompressor   mappingQuality;
	QualityScoreCompressor 	   queryQual;
	PairedEndCompressor 	   pairedEnd;
    OptionalFieldCompressor    optionalField;

	FILE *metaFile;
	FILE *outputFile;

	int  blockSize;

public:
	SAMFileCompressor (const std::string &outFile, const std::string &samFile, const std::string &genomeFile, int blockSize);
	~SAMFileCompressor (void);

private:
	void outputBlock (const vector<char> &out);

public:
	void compress (void);
};

class SAMFileDecompressor {
	ReferenceDecompressor 			reference;
	ReadNameDecompressor 			readName;
	MappingFlagDecompressor 		mappingFlag;
	MappingOperationDecompressor 	mappingOperation;
	MappingQualityDecompressor		mappingQuality;
	EditOperationDecompressor 		editOperation;
	QualityScoreDecompressor 		queryQual;
	PairedEndDecompressor 			pairedEnd;
    OptionalFieldDecompressor       optionalField;

	FILE *metaFile;
	FILE *samFile;
	FILE *inFile;

    int blockSize;

public:
	SAMFileDecompressor (const std::string &inFile, const std::string &outFile, const std::string &genomeFile, int bs);
	~SAMFileDecompressor (void);

private:
	bool getSingleBlock (vector<char> &in);
	bool getBlock (void);

public:
	void decompress (void);
};

#endif
