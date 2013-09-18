#ifndef ReadName_H
#define ReadName_H

#include <string>

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

const int MAX_TOKEN = 10;

class ReadNameCompressor: public Compressor {
	GenericCompressor<uint8_t, GzipCompressionStream<6> > 
		indexStream;
	GenericCompressor<uint8_t, GzipCompressionStream<6> > 
		contentStream;

	std::string prevTokens[MAX_TOKEN];
	char token;

public:
	ReadNameCompressor(int blockSize);
	virtual ~ReadNameCompressor(void);

public:
	void addRecord (const std::string &rn);
	void outputRecords (Array<uint8_t> &output);

private:
	void detectToken (const std::string &rn);
	void addTokenizedName (const std::string &rn);
};

class ReadNameDecompressor: public Decompressor {
	GenericDecompressor<uint8_t, GzipDecompressionStream> 
		indexStream;
	GenericDecompressor<uint8_t, GzipDecompressionStream> 
		contentStream;

	std::string prevTokens[MAX_TOKEN];
	char token;

public:
	ReadNameDecompressor(int blockSize);
	virtual ~ReadNameDecompressor(void);

public:
	std::string getRecord (void);
	bool hasRecord (void);
	void importRecords (uint8_t *in, size_t in_size);

private:
	void extractTokenizedName (std::string &s);
};

#endif
