#ifndef EditOperation_H
#define EditOperation_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/ArithmeticStream.h"
#include "../Engines/StringEngine.h"


typedef StringCompressor<GzipCompressionStream<6> >     EditOperationCompressor;
typedef StringDecompressor<GzipDecompressionStream> EditOperationDecompressor;


/*

struct CigarOp {
	string keys;
	std::vector<int> vals;
};

class EditOperationCompressor: public Compressor {
	// CompressionStream *editLenStream; --> stream
	CompressionStream *editKeyStream;
	CompressionStream *editValStream;
	CompressionStream *sequenceStream;

	std::vector<short> lengths;
	std::vector<char> keys;
	std::vector<int> values;

	std::string sequences;

public:
	EditOperationCompressor(int blockSize);
	~EditOperationCompressor(void);

public:
	void addRecord (const CigarOp &cop);
	void outputRecords (std::vector<char> &output);
};

class EditOperationDecompressor: public StringDecompressor<GzipDecompressionStream> {
	virtual const char *getID () const { return "EditOperation"; }

public:
	EditOperationDecompressor(int blockSize):
		StringDecompressor<GzipDecompressionStream>(blockSize) {}
};

*/
#endif
