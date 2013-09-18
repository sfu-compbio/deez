#include "GzipStream.h"

GzipDecompressionStream::GzipDecompressionStream (void) {
}

GzipDecompressionStream::~GzipDecompressionStream (void) {
}

size_t GzipDecompressionStream::decompress (uint8_t *source, size_t source_sz, 
	Array<uint8_t> &dest, size_t dest_offset) {
	size_t sz = dest.size() - dest_offset;
	int c = uncompress(dest.data() + dest_offset, &sz, source, source_sz);

	if (c == Z_OK) 
		return sz;
	else 
		throw DZException("zlib decompression error %d", c);
}
