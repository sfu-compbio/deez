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
extern bool optComment;

struct index_t {
	size_t startPos, endPos;
	size_t zpos, currentBlockCount;
	size_t fS, fE;
	vector<Array<uint8_t>> fieldData;

	index_t(): fieldData(8) { }
};
typedef pair<pair<int, string>, pair<size_t, size_t>> range_t;

class IFileDecompressor {
public:
   	vector<string> comments;
    virtual bool decompress2(const string &range, int filterFlag, bool cont) = 0;
};

class FileDecompressor: public IFileDecompressor {
	vector<SAMComment> samComment;
	vector<shared_ptr<SequenceDecompressor>> sequence;
	vector<shared_ptr<EditOperationDecompressor>> editOp;
	vector<shared_ptr<ReadNameDecompressor>> readName;
	vector<shared_ptr<MappingFlagDecompressor>> mapFlag;
	vector<shared_ptr<MappingQualityDecompressor>> mapQual;
	vector<shared_ptr<QualityScoreDecompressor>> quality;
	vector<shared_ptr<PairedEndDecompressor>> pairedEnd;
	vector<shared_ptr<OptionalFieldDecompressor>> optField;

   	vector<string> fileNames;
	vector<FILE*> samFiles;
	vector<map<string, map<size_t, index_t>>> indices;

	shared_ptr<File> inFile;
	uint32_t magic;
	gzFile idxFile;
	
	string genomeFile;
	string outFile;
	vector<shared_ptr<Stats>> stats;
	size_t inFileSz;
	size_t statPos;
    size_t blockSize;
    uint16_t numFiles;
   	vector<int> fileBlockCount;

   	bool finishedRange;

protected:
	const bool isAPI; // Ugly; hack for now
    virtual inline void printRecord(const string &rname, int flag, const string &chr, const EditOperation &eo, int mqual,
        const string &qual, const string &optional, const PairedEndInfo &pe, int file, int thread);

	inline void printRecord(const string &record, int file);
    virtual inline void printComment(int file);

public:
	void printStats (int filterFlag);

public:
	FileDecompressor (const std::string &inFile, const std::string &outFile, const std::string &genomeFile, int bs, bool isAPI = false);
	~FileDecompressor (void);

private:
	void getMagic (void);
	void getComment (void);
	size_t getBlock (int f, const std::string &chromosome, size_t start, size_t end, int filterFlag);
	void readBlock (Array<uint8_t> &in);
	void loadIndex (); 
	vector<range_t> getRanges (std::string range);

private: // TODO finish
	void query (const string &query, const string &range);

public:
	void decompress (int filterFlag);
	void decompress (const std::string &range, int filterFlag);
	virtual bool decompress2 (const string &range, int filterFlag, bool cont);
};

#endif // Decompress_H