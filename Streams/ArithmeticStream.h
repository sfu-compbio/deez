#ifndef ArithmeticStream_H
#define ArithmeticStream_H

#include <vector>
#include <zlib.h>
#include "../Common.h"
#include "Stream.h"

template<typename TModel>
class ArithmeticCompressionStream: public CompressionStream {
	TModel model;
	std::vector<char> *out;

	uint32_t lo;
	uint32_t hi;
	uint32_t underflow;

	char bufPos;
	char cOutput;

private:
	void setbit (int t);
	void writeByte (char c);
	void flush (void);

public:
	ArithmeticCompressionStream (void);
	~ArithmeticCompressionStream (void);

public:
	void compress (void *source, size_t sz, std::vector<char> &result);
};

template<typename TModel>
class ArithmeticDecompressionStream: public DecompressionStream {
private:
	TModel model;
	char *in;

	uint32_t lo;
	uint32_t hi;
	uint32_t code;

	char bufPos;
	char initialized;

private:
	char getbit (void);
	char readByte (void);

public:
	ArithmeticDecompressionStream (void);
	~ArithmeticDecompressionStream (void);

public:
	void decompress (void *source, size_t sz, std::vector<char> &result);
};

#include "ArithmeticStream.tcc"

#endif
