#include "FileGzipStream.h"

#define PATH "__qual__"

FileGzipCompressionStream::FileGzipCompressionStream (void) {
	stream = gzopen(PATH ".dz", "wb6");
	ustream = fopen(PATH ".orig", "wb");
}

FileGzipCompressionStream::~FileGzipCompressionStream (void) {
	gzclose(stream);
	fclose (ustream);
}

void FileGzipCompressionStream::compress (void *source, size_t sz, std::vector<char> &result) {
	gzwrite(stream, source, sz);
	fwrite(source, 1, sz, ustream);
	result.resize(sz); // eeee ...........................
}


FileGzipDecompressionStream::FileGzipDecompressionStream (void) {
	stream = gzopen(PATH, "rb");
	if (stream == Z_NULL)
		throw;
}

FileGzipDecompressionStream::~FileGzipDecompressionStream (void) {
	gzclose(stream);
}

void FileGzipDecompressionStream::decompress (void *source, size_t sz, std::vector<char> &result) {
	result.resize(sz);
	gzread(stream, &result[0], sz);
}
