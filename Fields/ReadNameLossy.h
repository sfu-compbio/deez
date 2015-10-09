#ifndef ReadNameLossy_H
#define ReadNameLossy_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
#include "../Engines/GenericEngine.h"

#if __cplusplus <= 199711L
	#include <tr1/unordered_map>
	typedef std::tr1::unordered_map<string, int32_t> ReadMap;
#else
	#include <unordered_map>
	typedef std::unordered_map<string, int32_t> ReadMap;
#endif


class ReadNameLossyCompressor: 
	public GenericCompressor<uint32_t, GzipCompressionStream<6> >  
{
	ReadMap mmap;

public:
	ReadNameLossyCompressor(int blockSize);
	virtual ~ReadNameLossyCompressor(void);

public:
	void addRecord (const string &s);
};

typedef GenericDecompressor<uint32_t, GzipDecompressionStream> 
	ReadNameLossyDecompressor;

#endif
