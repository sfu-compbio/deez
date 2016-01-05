#ifndef OptionalField_H
#define OptionalField_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/BzipStream.h"
#include "../Engines/StringEngine.h"
#include "EditOperation.h"
#include "SAMComment.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <set>

const int AlphabetStart = '!';
const int AlphabetEnd = '~';
const int AlphabetRange = AlphabetEnd - AlphabetStart + 1;
constexpr int OptTag(char A, char B, char t)
{
	return (A - '!') * AlphabetRange * AlphabetRange + (B - '!') * AlphabetRange + t - '!';
}
constexpr int OptTag(const char *F) 
{
	return OptTag(F[0], F[1], F[2]);
}
#define OptTagKey(X) OptTag(#X)
#define OptTagDef(X) static const int X = OptTag(#X)

inline string keyStr(int f) 
{
	char c[4] = {0};
	c[0] = AlphabetStart + (f / AlphabetRange) / AlphabetRange;
	c[1] = AlphabetStart + (f / AlphabetRange) % AlphabetRange;
	c[2] = AlphabetStart + f % AlphabetRange;
	return string(c);
}

struct OptionalField {
	string data;
	Array<pair<int, int>> keys; // Tag, Position | Int

	OptionalField(): keys(50, 100) 
	{
	}
	
	void parse1 (const char *rec, const char *recEnd, 
		std::unordered_map<int32_t, std::map<std::string, int>> &library);
	void parse(char *rec, const EditOperation &eo, 
		std::unordered_map<int32_t, std::map<std::string, int>> &library);

public:
	static std::string getXDfromMD(const std::string &eoMD);
private:
	bool parseXD(const char *rec, const std::string &eoMD);
};

class OptionalFieldCompressor: 
	public StringCompressor<GzipCompressionStream<6>>  
{
	std::map<int, shared_ptr<CompressionStream>> fieldStreams;
	vector<int> fields;
	int fieldCount;
	vector<int> prevIndex;
	vector<Array<uint8_t>*> oa;
	std::condition_variable condition;
	std::mutex conditionMutex;
	Array<int32_t> idxToKey;

public:
	OptTagDef(MDZ);
	OptTagDef(XDZ);
	OptTagDef(NMi);
	static std::unordered_set<int> LibraryTags;
	static std::unordered_set<int> QualityTags;

public:
	OptionalFieldCompressor (void);
	~OptionalFieldCompressor (void);
	
public:
	void outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k, 
		const Array<OptionalField> &optFields, std::unordered_map<int32_t, std::map<std::string, int>> &library);
	void compressThreads(Array<Array<uint8_t>> &outT, ctpl::thread_pool &pool);
	//void getIndexData (Array<uint8_t> &out);

	void printDetails(void);
	size_t compressedSize(void);

private:
	int processFields(const char *rec, std::vector<Array<uint8_t>*> &out,
		Array<uint8_t> &tags, const OptionalField &of, size_t k);

public:
	enum Fields {
		TAG, 
		TAGX,
		TAGLEN,
		ENUM_COUNT
	};
};

class OptionalFieldDecompressor: 
	public GenericDecompressor<OptionalField, GzipDecompressionStream> 
{
	std::unordered_map<int, shared_ptr<DecompressionStream>> fieldStreams;
	std::unordered_map<int, std::unordered_map<int, std::string>> library;
	vector<int> fields;
	vector<int> prevIndex;

	vector<Array<uint8_t>> data;
	vector<Array<size_t>> dataLoc;
	vector<int> positions;

	std::condition_variable condition;
	std::mutex conditionMutex;
	uint8_t *inputBuffer;

public:
	OptionalFieldDecompressor (int blockSize);
	
public:
	void getRecord(size_t i, const EditOperation &eo, std::string &record);
	void importRecords (uint8_t *in, size_t in_size);
	void decompressThreads(ctpl::thread_pool &pool);

private:
	OptionalField parseFields(uint8_t *&tags, uint8_t *&in);
};


#endif
