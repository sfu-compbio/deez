#ifndef ReadName_H
#define ReadName_H

#include <string>

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"
#include "../Engines/StringEngine.h"

// typedef StringCompressor<GzipCompressionStream<6> >    
// 	ReadNameCompressor;
// typedef StringDecompressor<GzipDecompressionStream> 
// 	ReadNameDecompressor;

const int MAX_TOKEN = 20;

class ReadNameCompressor: 
	public StringCompressor<GzipCompressionStream<6> >  
{
	CompressionStream *indexStream;
	std::string prevTokens[MAX_TOKEN];
	char prevCharTokens[MAX_TOKEN];
//	char token;

public:
	ReadNameCompressor(int blockSize);
	virtual ~ReadNameCompressor(void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out);
	size_t compressedSize(void) { 
		LOGN("[IDX %lu S %lu]", indexStream->getCount(), stream->getCount());
		return stream->getCount() + indexStream->getCount(); 
	}

private:
//	void detectToken (const std::string &rn);
	void addTokenizedName (const std::string &rn, Array<uint8_t> &content, Array<uint8_t> &index);
};

class ReadNameDecompressor: 
	public StringDecompressor<GzipDecompressionStream>  
{
	DecompressionStream *indexStream;
//	char token;

public:
	ReadNameDecompressor(int blockSize);
	virtual ~ReadNameDecompressor(void);

public:
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *in, size_t in_size);
};

#endif
