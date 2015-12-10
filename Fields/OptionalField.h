#ifndef OptionalField_H
#define OptionalField_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"
#include <vector>
#include <unordered_map>

class OptionalFieldCompressor: 
	public StringCompressor<GzipCompressionStream<6> >  
{
	CompressionStream *indexStream;
	std::unordered_map<std::string, CompressionStream*> fieldStreams;
	std::unordered_map<std::string, int> fields;

public:
	OptionalFieldCompressor (int blockSize);
	~OptionalFieldCompressor (void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	//void getIndexData (Array<uint8_t> &out);

	size_t compressedSize(void);
	void printDetails(void);

private:
	int processFields(const string &rec, std::vector<Array<uint8_t>> &out, Array<uint8_t> &tags);
};

class OptionalFieldDecompressor: 
	public StringDecompressor<GzipDecompressionStream> 
{
	DecompressionStream *indexStream;
	std::unordered_map<std::string, DecompressionStream*> fieldStreams;

public:
	OptionalFieldDecompressor (int blockSize);
	virtual ~OptionalFieldDecompressor (void);
	
public:
	void importRecords (uint8_t *in, size_t in_size);
};


#endif
