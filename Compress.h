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
	vector<shared_ptr<Parser>> parsers;
	vector<shared_ptr<SequenceCompressor>> sequence;
	vector<shared_ptr<EditOperationCompressor>> editOp;
	vector<shared_ptr<ReadNameCompressor>> readName;
	vector<shared_ptr<MappingFlagCompressor>> mapFlag;
	vector<shared_ptr<MappingQualityCompressor>> mapQual;
	vector<shared_ptr<QualityScoreCompressor>> quality;
	vector<shared_ptr<PairedEndCompressor>> pairedEnd;
	vector<shared_ptr<OptionalFieldCompressor>> optField;

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

	template<typename Compressor, typename... ExtraParams>
	static void compressBlock (Array<uint8_t>& out, Array<uint8_t>& idxOut, size_t k, Compressor *c, ExtraParams... params);
	void outputBlock (Array<uint8_t> &out, Array<uint8_t> &idxOut);

public:
	void compress (void);

private:
	void parser(size_t f, size_t, size_t);

	std::condition_variable recordsAvailable;
	std::mutex queueMutex;
	std::vector<Record> recordsQueue;
	std::vector<size_t> queuePosition;
	bool canAccept;
	size_t currentMemUsage(size_t f);
};

#endif // Compress_H
