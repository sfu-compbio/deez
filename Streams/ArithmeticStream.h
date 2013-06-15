#ifndef ArithmeticStream_H
#define ArithmeticStream_H

#include <vector>
#include <zlib.h>
#include "../Common.h"
#include "Stream.h"

#define CODER_SHELWIEN

template<typename TModel>
class ArithmeticCompressionStream: public CompressionStream {
	TModel model;
	std::vector<char> *out;

#ifdef CODER_SUBBOTIN
	uint32_t low, range;
#elif defined CODER_SHELWIEN
	uint64_t Range;
	uint64_t Low;

	uint32_t rc_FFNum;		// number of all-F dwords between Cache and Low
	uint32_t rc_LowH;		// high 32bits of Low
	uint32_t rc_Cache;		// cached low dword
	uint32_t rc_Carry;		// Low carry flag
#else
	uint32_t lo;
	uint32_t hi;
	uint32_t underflow;

	char bufPos;
	char cOutput;
#endif

private:
	void setbit (uint32_t t);
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

#ifdef CODER_SUBBOTIN
	uint32_t low;
	uint32_t range;
	uint32_t code;
#elif defined CODER_SHELWIEN
	uint64_t Range;
	uint64_t Low;
	uint64_t Code;

	uint32_t rc_FFNum;		// number of all-F dwords between Cache and Low
	uint32_t rc_LowH;		// high 32bits of Low
	uint32_t rc_Cache;		// cached low dword
	uint32_t rc_Carry;		// Low carry flag
#else
	uint32_t lo;
	uint32_t hi;
	uint32_t code;
	char cOutput;
#endif

	char initialized;

private:
	uint32_t getbit (void);
	char readByte (void);

public:
	ArithmeticDecompressionStream (void);
	~ArithmeticDecompressionStream (void);

public:
	void decompress (void *source, size_t sz, std::vector<char> &result);
};

#include "ArithmeticStream.tcc"

#endif
