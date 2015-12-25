#ifndef OptionalField_H
#define OptionalField_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"
#include "EditOperation.h"
#include "SAMComment.h"
#include <vector>
#include <unordered_map>

const int AlphabetStart = '!';
const int AlphabetEnd = '~';
const int AlphabetRange = AlphabetEnd - AlphabetStart + 1;
constexpr int Field(char A, char B, char t)
{
	return (A - '!') * AlphabetRange * AlphabetRange + (B - '!') * AlphabetRange + t - '!';
}
constexpr int Field(const char *F) 
{
	return Field(F[0], F[1], F[2]);
}
#define OptTag(X) const int X = Field(#X)

// Alignment tags: to be calculated
OptTag(MDZ);
OptTag(XDZ);
OptTag(NMi);

// Library tags: to be looked up
OptTag(PGZ);
OptTag(RGZ);
OptTag(LBZ);
OptTag(PUZ);
OptTag(PGi);
OptTag(RGi);
OptTag(LBi);
OptTag(PUi);

// Quality tags: to be compressed with rANS
OptTag(BQZ);
OptTag(CQZ);
OptTag(E2Z);
OptTag(OQZ);
OptTag(QTZ);
OptTag(Q2Z);
OptTag(U2Z);

struct OptionalField {
	string data;
	int posNM, posMD, posXD;

	OptionalField(): posXD(-1), posMD(-1), posNM(-1) {}
};

class OptionalFieldCompressor: 
	public StringCompressor<GzipCompressionStream<6>>  
{
	std::map<std::string, shared_ptr<CompressionStream>> fieldStreams;
	std::unordered_map<std::string, int> PG, RG;

	vector<int> fields;
	int fieldCount;

	int totalXD, failedXD;
	int totalNM, failedNM;
	int totalMD, failedMD;

public:
	OptionalFieldCompressor (void);
	~OptionalFieldCompressor (void);
	
public:
	void outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k, const Array<EditOperation> &editOps);
	//void getIndexData (Array<uint8_t> &out);

	void printDetails(void);
	size_t compressedSize(void);

public:
	static std::string getXDfromMD(const std::string &eoMD);

private:
	int processFields(const char *rec, const char *recEnd, std::vector<Array<uint8_t>> &out, Array<uint8_t> &tags, const EditOperation &eo);
	bool parseXD(const char *rec, const string &eoMD);
};

class OptionalFieldDecompressor: 
	public GenericDecompressor<OptionalField, GzipDecompressionStream> 
{
	std::unordered_map<int, shared_ptr<DecompressionStream>> fieldStreams;
	std::unordered_map<int, std::string> PG, RG;
	vector<int> fields;

public:
	OptionalFieldDecompressor (int blockSize);
	virtual ~OptionalFieldDecompressor (void);
	
public:
	const OptionalField &getRecord(const EditOperation &eo);
	void importRecords (uint8_t *in, size_t in_size);

private:
	OptionalField parseFields(int size, uint8_t *&tags, uint8_t *&in, std::vector<Array<uint8_t>>& oa, std::vector<size_t> &out);
};


#endif
