#ifndef GzipStream_H
#define GzipStream_H

#include <zlib.h>
#include "../Common.h"
#include "Stream.h"

template<char Level>
class GzipCompressionStream: public CompressionStream {	
public:
	GzipCompressionStream (void) {}
	~GzipCompressionStream (void) {}

public:
	virtual size_t compress (uint8_t *source, size_t source_sz, 
		Array<uint8_t> &dest, size_t dest_offset) 
	{
		if (dest_offset + compressBound(source_sz) >= dest.size())
			dest.resize(dest_offset + compressBound(source_sz) + 1);

		size_t sz = dest.size() - dest_offset;
		int c = compress2(dest.data() + dest_offset, &sz, source, source_sz, Level);
		if (c == Z_OK) 
			return sz;
		else 
			throw DZException("zlib compression error %d", c);
	}
	virtual void getCurrentState (Array<uint8_t> &ou) {}
};

class GzipDecompressionStream: public DecompressionStream {
public:
	GzipDecompressionStream (void);
	~GzipDecompressionStream (void);

public:
	virtual size_t decompress (uint8_t *source, size_t source_sz, 
		Array<uint8_t> &dest, size_t dest_offset); 
	virtual void setCurrentState (uint8_t *source, size_t source_sz) {}
};

#endif
