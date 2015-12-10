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
	size_t ospan, opos;
	int flag;
	
	PairedEndInfo (void) {}
	PairedEndInfo (const std::string &c, size_t p, int32_t t, size_t op, size_t os, int f):
		chr(c), pos(p), tlen(t), opos(op), ospan(os), flag(f)
	{
		/*if ( (mc == "=") && !((mpos <= pos && tlen <= 0) || (mpos > pos && tlen >= 0)) )
			throw DZException("Mate position and template length inconsistent! pos=%lu matepos=%lu tlen=%d", mpos, pos, tlen);
		if (mc == "=") {
			if (tlen <= 0)
				pos -= mpos;
			else
				pos = mpos - pos;
		}
		pos++;*/
	}
};

class PairedEndCompressor: 
	public GenericCompressor<PairedEndInfo, GzipCompressionStream<6> > 
{
	CompressionStream *chromosomeStream;
	CompressionStream *tlenStream[5];
	CompressionStream *tlenBitStream;


public:
	PairedEndCompressor (int blockSize);
	virtual ~PairedEndCompressor (void);

	size_t compressedSize(void) { 
		LOGN("[RNEXT %'lu TLEN %'lu TLEN2 %'lu TLEN_Bit %'lu RPOS %'lu]\n", 
			chromosomeStream->getCount(),
			tlenStream[0]->getCount(),
			tlenStream[1]->getCount(),
			tlenBitStream->getCount(),
			stream->getCount()
		);
		return stream->getCount() + tlenStream[0]->getCount() +
			chromosomeStream->getCount() + tlenBitStream->getCount();
	}

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
	const PairedEndInfo &getRecord (const std::string &mc, size_t mpos);
};

#endif

