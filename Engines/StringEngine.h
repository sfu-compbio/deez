#ifndef StringEngine_H
#define StringEngine_H

#include <deque>
#include <string>

#include "../Common.h"
#include "GenericEngine.h"

template<typename TStream>
class StringCompressor: public GenericCompressor<std::string, TStream> {
	virtual const char *getID () const { return "String"; }

public:
	StringCompressor (int blockSize);
	virtual ~StringCompressor (void);

public:
	virtual void outputRecords (std::vector<char> &output);
};

template<typename TStream>
class StringDecompressor: public GenericDecompressor<std::string, TStream> {
	virtual const char *getID () const { return "String"; }

public:
	StringDecompressor (int blockSize);
	virtual ~StringDecompressor (void);
	
public:
	virtual void importRecords (const std::vector<char> &input);
};

#include "StringEngine.tcc"

#endif
