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

	enum Bits {
		LOOK_AHEAD,
		LOOK_AHEAD_1, // for some incorrect TLEN estimates
		LOOK_BACK,
		OK,
		GREATER_THAN_0,
		LESS_THAN_0
	};
	
	PairedEndInfo (void) {}
	PairedEndInfo (const std::string &c, size_t pos, int32_t t, size_t opos, size_t ospan, bool reverse);
};
template<>
size_t sizeInMemory(PairedEndInfo t);


class PairedEndCompressor: 
	public GenericCompressor<PairedEndInfo, GzipCompressionStream<6>> 
{
public:
	PairedEndCompressor (void);
	
public:
	void outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k, const Array<PairedEndInfo> &pairedEndInfos);
	void printDetails(void);

public:
	enum Fields {
		DIFF,
		CHROMOSOME,
		TLEN,
		TLENBIT,
		ENUM_COUNT
	};
};

class PairedEndDecompressor: 
	public GenericDecompressor<PairedEndInfo, GzipDecompressionStream> 
{
public:
	PairedEndDecompressor (int blockSize);
	virtual ~PairedEndDecompressor (void);
	
public:
	void importRecords (uint8_t *in, size_t in_size);
	PairedEndInfo &getRecord (size_t i, size_t opos, size_t ospan, bool reverse);
};

#endif

