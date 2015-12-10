#ifndef PairedEnd_H
#define PairedEnd_H

#include <string>
#include <map>

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

struct PairedEndInfo {
	std::string chr;
	int32_t tlen, diff;
	size_t pos;
	char bit;
	
	PairedEndInfo (void) {}
	PairedEndInfo (const std::string &c, size_t pos, int32_t t, size_t opos, size_t ospan, bool reverse):
		chr(c), tlen(t), bit(-1), pos(pos)
	{
		if ((tlen < 0) == reverse) {
			bit = 0;
		} else {
			bit = 1 + (tlen < 0);
		}
		if (tlen > 0) { // replace opos with size
			diff = opos + tlen - ospan - pos;
		} else {
			diff = opos + tlen + ospan - pos;
		} 
		//LOG("%d %d %d %d %s", tlen, pos, diff, bit, chr.c_str());
	}
};

class PairedEndCompressor: 
	public GenericCompressor<PairedEndInfo, GzipCompressionStream<6> > 
{
	std::vector<CompressionStream*> streams;

public:
	PairedEndCompressor (int blockSize);
	virtual ~PairedEndCompressor (void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	size_t compressedSize(void);
	void printDetails(void);
};

class PairedEndDecompressor: 
	public GenericDecompressor<PairedEndInfo, GzipDecompressionStream> 
{
	std::vector<DecompressionStream*> streams;

public:
	PairedEndDecompressor (int blockSize);
	virtual ~PairedEndDecompressor (void);
	
public:
	void importRecords (uint8_t *in, size_t in_size);
	const PairedEndInfo &getRecord (size_t opos, size_t ospan, bool reverse);
};

#endif

