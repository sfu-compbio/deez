#ifndef OptionalField_H
#define OptionalField_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"
#include "EditOperation.h"
#include "SAMComment.h"
#include <vector>
#include <unordered_map>

struct OptionalField {
	string data;
	int posNM, posMD, posXD;

	OptionalField(): posXD(-1), posMD(-1), posNM(-1) {}
};

class OptionalFieldCompressor: 
	public StringCompressor<GzipCompressionStream<6>>  
{
	std::map<std::string, shared_ptr<CompressionStream>> fieldStreams;
	std::unordered_map<std::string, int> fields;
	std::unordered_map<std::string, int> PG, RG;

	int totalXD, failedXD;
	int totalNM, failedNM;
	int totalMD, failedMD;

public:
	OptionalFieldCompressor (void);
	~OptionalFieldCompressor (void);
	
public:
	void outputRecords (const CircularArray<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k, const CircularArray<EditOperation> &editOps);
	//void getIndexData (Array<uint8_t> &out);

	void printDetails(void);
	size_t compressedSize(void);

public:
	static std::string getXDfromMD(const std::string &eoMD);

private:
	int processFields(const string &rec, std::vector<Array<uint8_t>> &out, Array<uint8_t> &tags, const EditOperation &eo);

	void parseMD(const string &rec, int &i, const string &eoMD, Array<uint8_t> &out);
	void parseXD(const string &rec, int &i, const string &eoMD, Array<uint8_t> &out);
};

class OptionalFieldDecompressor: 
	public GenericDecompressor<OptionalField, GzipDecompressionStream> 
{
	std::unordered_map<std::string, shared_ptr<DecompressionStream>> fieldStreams;
	std::unordered_map<int, std::string> fields;
	std::unordered_map<int, std::string> PG, RG;

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
