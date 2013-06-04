#include "GzipStream.h"
using namespace std;

GzipDecompressionStream::GzipDecompressionStream (void) {
	stream.zalloc = Z_NULL;
	stream.zfree  = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = 0;
	stream.next_in  = Z_NULL;
	if (inflateInit(&stream) != Z_OK)
		throw;
	buffer = new char[CHUNK];
}

GzipDecompressionStream::~GzipDecompressionStream (void) {
	inflateEnd(&stream);
	delete[] buffer;
}

void GzipDecompressionStream::decompress (void *source, size_t sz, vector<char> &result) {
	stream.next_in = (unsigned char*)source;
	stream.avail_in = sz;
	if (stream.avail_in == 0)
		return;

	do {
		stream.avail_out = CHUNK;
		stream.next_out = (unsigned char*)buffer;
		int ret = inflate(&stream, Z_NO_FLUSH);
		if (ret != Z_STREAM_END  && ret != Z_OK)
			throw;

		size_t have = CHUNK - stream.avail_out;
		result.insert(result.end(), buffer, buffer + have);
	} while (stream.avail_out == 0);
}
