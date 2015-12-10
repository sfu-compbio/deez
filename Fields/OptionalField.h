#ifndef OptionalField_H
#define OptionalField_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"

// typedef StringCompressor<GzipCompressionStream<6> >    
// 	OptionalFieldCompressor;
typedef StringDecompressor<GzipDecompressionStream> 
 	OptionalFieldDecompressor;


class OptionalFieldCompressor: 
	public StringCompressor<GzipCompressionStream<6> >  
{
	CompressionStream *indexStream;
	std::vector<CompressionStream*> fieldStreams;
	std::map<std::string, int> fields;

public:
	OptionalFieldCompressor (int blockSize);
	~OptionalFieldCompressor (void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out);

	size_t compressedSize(void) { 
		LOGN("[FieldIndex %'lu ", indexStream->getCount());
			int res = 0;
		for (auto &m: fields) {
			res += fieldStreams[m.second]->getCount();
			LOGN("%s %'lu ", m.first.c_str(), fieldStreams[m.second]->getCount());
		}
		LOGN("]\n");
		return indexStream->getCount() + res; 
	}

private:
	bool tokenize(const string &rec, vector<Array> &out, Array<uint8_t> &tags);
};

/*
class OptionalFieldDecompressor: public Compressor {
	DecompressionStream *keyStream;

	std::vector<std::string> fieldKey;	// tags
	std::vector<char> fieldType;		// types
	std::vector<std::deque<std::string> > fieldData;	// tag contents
	std::vector<std::vector<short> > records;			// tag list for each record
	int recordCount;

public:
	OptionalFieldDecompressor (int blockSize);
	~OptionalFieldDecompressor (void);

public:
	std::string getRecord (void);
	bool hasRecord (void);
	void importRecords (const std::vector<char> &input);
};
*/

#endif
