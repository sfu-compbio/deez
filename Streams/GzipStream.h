#ifndef GzipStream_H
#define GzipStream_H

#include <vector>
#include <zlib.h>
#include "../Common.h"
#include "Stream.h"

static const int CHUNK = 256 * KB;

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

template<char Level>
class GzipDecompressionStream_: public DecompressionStream {
	z_stream stream;
	char *buffer;

public:
	GzipDecompressionStream_ (void);
	~GzipDecompressionStream_ (void);

public:
	void decompress (void *source, size_t sz, std::vector<char> &result);
};

#include "GzipStream.tcc"

typedef GzipDecompressionStream_<6> GzipDecompressionStream;

#endif
