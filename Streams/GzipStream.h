#ifndef GzipStream_H
#define GzipStream_H

#include <vector>
#include <zlib.h>
#include "../Common.h"
#include "Stream.h"

const int CHUNK = 256 * KB;

template<char Level>
class GzipCompressionStream: public CompressionStream {
	z_stream stream;
	char *buffer;

public:
	GzipCompressionStream (void);
	~GzipCompressionStream (void);

public:
	void compress (void *source, size_t sz, std::vector<char> &result);
};

class GzipDecompressionStream: public DecompressionStream {
	z_stream stream;
	char *buffer;

public:
	GzipDecompressionStream (void);
	~GzipDecompressionStream (void);

public:
	void decompress (void *source, size_t sz, std::vector<char> &result);
};

template<char Level>
GzipCompressionStream<Level>::GzipCompressionStream (void) {
	stream.zalloc = Z_NULL;
	stream.zfree  = Z_NULL;
	stream.opaque = Z_NULL;
	if (deflateInit(&stream, Level) != Z_OK)
		throw;
	buffer = new char[CHUNK];
}

template<char Level>
GzipCompressionStream<Level>::~GzipCompressionStream (void) {
	deflateEnd(&stream);
	delete[] buffer;
}

template<char Level>
void GzipCompressionStream<Level>::compress (void *source, size_t sz, std::vector<char> &result) {
	stream.avail_in = sz;
	stream.next_in = (unsigned char*)source;

	do {
		stream.avail_out = CHUNK;
		stream.next_out = (unsigned char*)buffer;
		if (deflate(&stream, Z_SYNC_FLUSH) == Z_STREAM_ERROR)
			throw;
		size_t have = CHUNK - stream.avail_out;
		result.insert(result.end(), buffer, buffer + have);
	} while (stream.avail_out == 0);
	if (stream.avail_in != 0)
		throw;
}

#endif
