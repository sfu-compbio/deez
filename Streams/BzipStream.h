#ifndef BzipStream_H
#define BzipStream_H

#include <bzlib.h>
#include "../Common.h"
#include "Stream.h"

class BzipCompressionStream: public CompressionStream {	
	size_t bz2compressBound(size_t sz) {
		return double(sz * 1.1) + 600;
	}

public:
	virtual size_t compress (uint8_t *source, size_t source_sz, 
		Array<uint8_t> &dest, size_t dest_offset) 
	{
		if (dest_offset + bz2compressBound(source_sz) >= dest.size())
			dest.resize(dest_offset + bz2compressBound(source_sz) + 1);

		unsigned int sz = dest.size() - dest_offset;
		int c = BZ2_bzBuffToBuffCompress((char*)dest.data() + dest_offset, &sz, (char*)source, source_sz, 9, 0, 0);
		if (c == BZ_OK) {
			this->compressedCount += sz;
			return sz;
		}
		else 
			throw DZException("bzlib compression error %d, %d", c, source_sz);
	}

	virtual void getCurrentState (Array<uint8_t> &ou) 
	{
	}
};

class BzipDecompressionStream: public DecompressionStream {
public:
	virtual size_t decompress (uint8_t *source, size_t source_sz, 
		Array<uint8_t> &dest, size_t dest_offset) 
	{
		unsigned int sz = dest.size() - dest_offset;
		int c = BZ2_bzBuffToBuffDecompress((char*)dest.data() + dest_offset, &sz, (char*)source, source_sz, 0, 0);

		if (c == BZ_OK) 
			return sz;
		else 
			throw DZException("bzlib decompression error %d", c);
	}

	virtual void setCurrentState (uint8_t *source, size_t source_sz) 
	{
	}
};

#endif
