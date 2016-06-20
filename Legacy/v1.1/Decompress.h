#pragma once

#include "Common.h"
#include "FileIO.h"
#include "Fields.h"
#include "Streams.h"
#include <map>
#include <vector>
#include <unordered_map>

#include "../../Decompress.h"

namespace Legacy {
namespace v11 {

struct index_t {
	size_t startPos, endPos;
	size_t zpos, currentBlockCount;
	size_t fS, fE;
	vector<Array<uint8_t>> fieldData;

	index_t(): fieldData(8) { }
};
typedef pair<pair<int, string>, pair<size_t, size_t>> range_t;

class Stats {
public:
	static const int FLAGCOUNT = (1 << 16) - 1;
	
private:
	size_t flags[FLAGCOUNT];
	size_t reads;

public:
	std::string fileName;
	std::map<std::string, Reference::Chromosome> chromosomes;

public:
	Stats();
	Stats(Array<uint8_t> &in, uint32_t);
	~Stats();

public:
	void addRecord (uint16_t flag);
	size_t getStats (int flag);

	size_t getReadCount() { return reads; }
	size_t getChromosomeCount() { return chromosomes.size(); }
	size_t getFlagCount(int i) { return flags[i]; }
};

class FileDecompressor: public IFileDecompressor {
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

	File *inFile;
	uint32_t magic;
	gzFile idxFile;
	
	string genomeFile;
	string outFile;
	Stats *stats;
	size_t inFileSz;
    size_t blockSize;
   	vector<int> fileBlockCount;

   	bool finishedRange;

protected:
    virtual inline void printRecord(const string &rname, int flag, const string &chr, const EditOperation &eo, int mqual, const string &qual, const string &optional, const PairedEndInfo &pe, int file, int thread);
    virtual inline void printComment(int file);

public:
	static void printStats (const std::string &inFile, int filterFlag);

public:
	FileDecompressor (const std::string &inFile, const std::string &outFile, const std::string &genomeFile, int bs);
	~FileDecompressor (void);

private:
	void getMagic (void);
	void getComment (void);
	size_t getBlock (int f, const std::string &chromosome, size_t start, size_t end, int filterFlag);
	void readBlock (Decompressor *d, Array<uint8_t> &in);
	void loadIndex (); 
	vector<range_t> getRanges (std::string range);

private: // TODO finish
	void query (const string &query, const string &range);

public:
    void decompress (int filterFlag);
	void decompress (const std::string &range, int filterFlag);

	virtual bool decompress2 (const string &range, int filterFlag, bool cont);
};

};
};
