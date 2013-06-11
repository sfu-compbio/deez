#ifndef FileGzipStream_H
#define FileGzipStream_H

#include <vector>
#include <zlib.h>
#include <stdio.h>
#include "../Common.h"
#include "Stream.h"

class FileGzipCompressionStream: public CompressionStream {
	gzFile stream;
	FILE *ustream;

public:
	FileGzipCompressionStream (void);
	~FileGzipCompressionStream (void);

public:
	void compress (void *source, size_t sz, std::vector<char> &result);
};

class FileGzipDecompressionStream: public DecompressionStream {
	gzFile stream;

public:
	FileGzipDecompressionStream (void);
	~FileGzipDecompressionStream (void);

public:
	void decompress (void *source, size_t sz, std::vector<char> &result);
};

#endif
