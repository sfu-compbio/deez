#ifndef OptionalField_H
#define OptionalField_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
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

struct OptionalField {
	string data;
	int posNM, posMD, posXD;

	Array<pair<int, int>> keys; // Tag, Position | Int

	OptionalField(): posXD(-1), posMD(-1), posNM(-1), keys(50, 100) {}
	
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
	//void getIndexData (Array<uint8_t> &out);

	void printDetails(void);
	size_t compressedSize(void);

private:
	int processFields(const char *rec, std::vector<Array<uint8_t>> &out,
		Array<uint8_t> &tags, const OptionalField &of, size_t k);
};

class OptionalFieldDecompressor: 
	public GenericDecompressor<OptionalField, GzipDecompressionStream> 
{
	std::unordered_map<int, shared_ptr<DecompressionStream>> fieldStreams;
	std::unordered_map<int, std::unordered_map<int, std::string>> library;
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
