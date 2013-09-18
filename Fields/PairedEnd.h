#ifndef PairedEnd_H
#define PairedEnd_H

#include <string>
#include <map>

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

struct PairedEndInfo {
	size_t pos;
	int32_t tlen;
	string chr;
};

class PairedEndCompressor: 
	public GenericCompressor<uint8_t, GzipCompressionStream<6> > {
	
	std::map<std::string, char> chromosomes;

public:
	PairedEndCompressor (int blockSize);
	virtual ~PairedEndCompressor (void);

public:
	void addRecord (const std::string &chr, size_t pos, int32_t len);
};

class PairedEndDecompressor: 
	public GenericDecompressor<uint8_t, GzipDecompressionStream> {

	std::map<char, std::string> chromosomes;

public:
	PairedEndDecompressor (int blockSize);
	virtual ~PairedEndDecompressor (void);
	
public:
	PairedEndInfo getRecord (void);
};

#endif

