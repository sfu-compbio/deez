#ifndef PairedEnd_H
#define PairedEnd_H

#include <string>
#include <map>

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

struct PairedEndInfo {
	std::string chr;
	size_t pos;
	int32_t tlen;

	PairedEndInfo (void) {}
	PairedEndInfo (const std::string &c, size_t p, int32_t t):
		chr(c), pos(p), tlen(t) {}
};

class PairedEndCompressor: 
	public GenericCompressor<PairedEndInfo, GzipCompressionStream<6> > 
{
public:
	PairedEndCompressor (int blockSize);
	virtual ~PairedEndCompressor (void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
};

class PairedEndDecompressor: 
	public GenericDecompressor<PairedEndInfo, GzipDecompressionStream> 
{
public:
	PairedEndDecompressor (int blockSize);
	virtual ~PairedEndDecompressor (void);
	
public:
	void importRecords (uint8_t *in, size_t in_size);
};

#endif

