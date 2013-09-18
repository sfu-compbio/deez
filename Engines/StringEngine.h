#ifndef StringEngine_H
#define StringEngine_H

#include <string>

#include "../Common.h"
#include "GenericEngine.h"

template<typename TStream>
class StringCompressor: public GenericCompressor<char, TStream> {
public:
	StringCompressor (int blockSize);
	virtual ~StringCompressor (void);

public:
	virtual void addRecord (const std::string &rec);
};

template<typename TStream>
class StringDecompressor: public GenericDecompressor<char, TStream> {
public:
	StringDecompressor (int blockSize);
	virtual ~StringDecompressor (void);
	
public:
	virtual const char *getRecord (void);
};

#include "StringEngine.tcc"

#endif
