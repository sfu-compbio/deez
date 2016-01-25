#ifndef GzipStream_H
#define GzipStream_H

#include <zlib.h>
#include "../Common.h"

// Z_RLE strategy inspired by Scramble
// http://sourceforge.net/projects/staden/files/io_lib/

template<char Level, int Strategy = Z_DEFAULT_STRATEGY>
class GzipCompressionStream: public CompressionStream {	
	static int compress (uint8_t *dest, size_t *destLen, uint8_t *source, size_t sourceLen)
	{
		z_stream stream;
		int err;

		stream.next_in = source;
		stream.avail_in = sourceLen;
		stream.next_out = dest;
		stream.avail_out = *destLen;
		if (stream.avail_out != *destLen) 
			return Z_BUF_ERROR;

		stream.zalloc = 0;
		stream.zfree = 0;
		stream.opaque = 0;

		err = deflateInit2(&stream, Level, Z_DEFLATED, 15, 8, Strategy);
		if (err != Z_OK) 
			return err;

		err = deflate(&stream, Z_FINISH);
		if (err != Z_STREAM_END) {
			deflateEnd(&stream);
			return err == Z_OK ? Z_BUF_ERROR : err;
		}
		*destLen = stream.total_out;

		err = deflateEnd(&stream);
		return err;
	}

public:
	virtual size_t compress (uint8_t *source, size_t source_sz, 
		Array<uint8_t> &dest, size_t dest_offset) 
	{
		if (dest_offset + compressBound(source_sz) >= dest.size())
			dest.resize(dest_offset + compressBound(source_sz) + 1);

		size_t sz = dest.size() - dest_offset;
		int c = GzipCompressionStream::compress(dest.data() + dest_offset, &sz, source, source_sz);
		if (c == Z_OK) {
			this->compressedCount += sz;
			return sz;
		}
		else 
			throw DZException("zlib compression error %d", c);
	}

	virtual void getCurrentState (Array<uint8_t> &ou) 
	{
	}
};

class GzipDecompressionStream: public DecompressionStream {
public:
	virtual size_t decompress (uint8_t *source, size_t source_sz, 
		Array<uint8_t> &dest, size_t dest_offset)
	{
		size_t sz = dest.size() - dest_offset;

		int c = uncompress(dest.data() + dest_offset, &sz, source, source_sz);

		if (c == Z_OK) 
			return sz;
		else 
			throw DZException("zlib decompression error %d", c);
	}

	virtual void setCurrentState (uint8_t *source, size_t source_sz) 
	{
	}
};

#endif
