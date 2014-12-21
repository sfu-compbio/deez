#ifndef ReadNameLossy_H
#define ReadNameLossy_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
#include "../Engines/GenericEngine.h"

class ReadNameLossyCompressor: 
	public GenericCompressor<uint32_t, GzipCompressionStream<6> >  
{
public:
	ReadNameLossyCompressor(int blockSize);
	virtual ~ReadNameLossyCompressor(void);

public:
	void addRecord (const string &s);
};

typedef GenericDecompressor<uint32_t, GzipDecompressionStream> 
	ReadNameLossyDecompressor;

#endif
