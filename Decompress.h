#ifndef Decompress_H
#define Decompress_H

#include "Common.h"
#include "Stats.h"
#include "FileIO.h"
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

#if __cplusplus <= 199711L
#include <tr1/unordered_map>
#else
#include <unordered_map>
#endif

extern size_t optBlock;

struct index_t {
	size_t startPos, endPos;
	size_t zpos, currentBlockCount;
	size_t fS, fE;
	vector<Array<uint8_t>> fieldData;

	index_t(): fieldData(8) { }
};
typedef pair<pair<int, string>, pair<size_t, size_t>> range_t;

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

	shared_ptr<File> inFile;
	uint32_t magic;
	gzFile idxFile;
	
	string genomeFile;
	string outFile;
	shared_ptr<Stats> stats;
	size_t inFileSz;
    size_t blockSize;
   	vector<int> fileBlockCount;

   	bool finishedRange;

protected:
    virtual inline void printRecord(const string &rname, int flag, const string &chr, const EditOperation &eo, int mqual,
        const string &qual, const string &optional, const PairedEndInfo &pe, int file)
    {
        fprintf(samFiles[file], "%s\t%d\t%s\t%zu\t%d\t%s\t%s\t%lu\t%d\t%s\t%s",
                rname.c_str(),
                flag,
                chr.c_str(),
                eo.start,
                mqual,
                eo.op.c_str(),
                pe.chr.c_str(),
                pe.pos,
                pe.tlen,
                eo.seq.c_str(),
                qual.c_str()
        );
        if (optional.size())
            fprintf(samFiles[file], "\t%s", optional.c_str());
        fprintf(samFiles[file], "\n");
    }

   	vector<string> comments;
    virtual inline void printComment(int file) {
        fwrite(comments[file].c_str(), 1, comments[file].size(), samFiles[file]);
    }

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
};

#endif // Decompress_H