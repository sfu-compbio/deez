#ifndef StringEngine_H
#define StringEngine_H

#include <deque>

#include "../Common.h"
#include "GenericEngine.h"

template<typename TStream>
class StringCompressor: public GenericCompressor<std::string, TStream> {
public:
	StringCompressor (int blockSize);

public:
	virtual void outputRecords (std::vector<char> &output);
};

template<typename TStream>
class StringDecompressor: public GenericDecompressor<std::string, TStream> {
public:
	StringDecompressor (int blockSize);
	
public:
	virtual void importRecords (const std::vector<char> &input);
};

#include "StringEngine.tcc"

#endif
