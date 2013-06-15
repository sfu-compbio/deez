#ifndef OptionalField_H
#define OptionalField_H

#include "../Common.h"
#include "../Streams/Stream.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"

typedef StringCompressor<GzipCompressionStream<6> >    OptionalFieldCompressor;
typedef StringDecompressor<GzipDecompressionStream> OptionalFieldDecompressor;

/*
class OptionalFieldCompressor: public Compressor {
	CompressionStream *keyStream;

	std::vector<short> fieldIdx;		// indices for tags
	std::vector<std::string> fieldKey;	// tags
	std::vector<char> fieldType;		// types
	std::vector<std::vector<std::string> > fieldData;	// tag contents
	std::vector<std::vector<short> > records;			// tag list for each record

public:
	OptionalFieldCompressor (int blockSize);
	~OptionalFieldCompressor (void);

public:
	void addRecord (const std::string &rec);
	void outputRecords (std::vector<char> &output);
};

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
