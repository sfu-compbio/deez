#include "FileGzipStream.h"
using namespace std;

FileGzipCompressionStream::FileGzipCompressionStream (void) {
	string s = getFilename();
	stream = gzopen(s.append(".dz").c_str(), "wb6");
	ustream = fopen(s.append(".orig").c_str(), "wb");
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
	stream = gzopen("", "rb");
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
