#ifndef Decompress_H
#define Decompress_H

#include "Common.h"
#include "Stats.h"
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

#include <map>
#include <tr1/unordered_map>

struct index_t {
	size_t startPos, endPos;
	size_t zpos, currentBlockCount;
	size_t fS, fE;
	vector<Array<uint8_t>> fieldData;

	index_t(): fieldData(8) { }
};

class FileDecompressor {
	vector<SequenceDecompressor*> sequence;
	vector<EditOperationDecompressor*> editOp;
	vector<ReadNameDecompressor*> readName;
	vector<MappingFlagDecompressor*> mapFlag;
	vector<MappingQualityDecompressor*> mapQual;
	vector<QualityScoreDecompressor*> quality;
	vector<PairedEndDecompressor*> pairedEnd;
	vector<OptionalFieldDecompressor*> optField;

	vector<string> fileNames;

	vector<FILE*> samFiles;
	vector<map<string, map<size_t, index_t>>> indices;

	FILE *inFile;
	gzFile idxFile;
	
	string genomeFile;
	string outFile;
	Stats *stats;
	size_t inFileSz;
    size_t blockSize;

public:
	static void printStats (const std::string &inFile, int filterFlag);

public:
	FileDecompressor (const std::string &inFile, const std::string &outFile, const std::string &genomeFile, int bs);
	~FileDecompressor (void);

private:
	void getMagic (void);
	void getComment (bool output);
	size_t getBlock (int f, const std::string &chromosome, size_t start, size_t end, int filterFlag);
	void readBlock (Decompressor *d, Array<uint8_t> &in);
	std::vector<int> loadIndex (bool inMemory);
	vector<pair<pair<int, string>, pair<size_t, size_t>>>
		getRanges (std::string range);

public:
	void query (const string &query, const string &range);
	void decompress (int filterFlag);
	void decompress (const std::string &idxFilePath, const std::string &range, int filterFlag);
};

#endif // Decompress_H