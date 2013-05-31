#ifndef SAMFile_H
#define SAMFile_H

#include "Common.h"
#include "Parsers/SAMParser.h"
#include "Engines/Reference.h"
#include "Engines/ReadName.h"
#include "Engines/MappingFlag.h"
#include "Engines/MappingOperation.h"
#include "Engines/MappingQuality.h"
#include "Engines/EditOperation.h"
#include "Engines/QualityScore.h"
#include "Engines/PairedEnd.h"
#include "Engines/OptionalField.h"

class SAMFileCompressor: public Compressor {
	SAMParser parser;

	ReferenceCompressor 	   reference;
	ReadNameCompressor 		   readName;
	MappingFlagCompressor 	   mappingFlag;
	MappingOperationCompressor mappingOffset;
    MappingQualityCompressor   mappingQuality;
//	EditOperationCompressor    editOperation;
	QualityScoreCompressor 	   queryQual;
	PairedEndCompressor 	   pairedEnd;
    OptionalFieldCompressor    optionalField;

	std::vector<EditOP> mapinfo;
	FILE *metaFile;
	int  blockSize;

public:
	SAMFileCompressor (const std::string &outFile, const std::string &samFile, const std::string &genomeFile, int blockSize);
	~SAMFileCompressor (void);

public:
	void compress (void);
};

class SAMFileDecompressor: public Decompressor {
	ReferenceDecompressor 			reference;
	ReadNameDecompressor 			readName;
	MappingFlagDecompressor 		mappingFlag;
	MappingOperationDecompressor 	mappingOffset;
	MappingQualityDecompressor		mappingQuality;
	EditOperationDecompressor 		editOperation;
	QualityScoreDecompressor 		queryQual;
	PairedEndDecompressor 			pairedEnd;
    OptionalFieldDecompressor       optionalField;

	FILE *metaFile;
	FILE *samFile;

    int blockSize;

public:
	SAMFileDecompressor (const std::string &inFile, const std::string &outFile, const std::string &genomeFile, int bs);
	~SAMFileDecompressor (void);

public:
	void decompress (void);
};

#endif
