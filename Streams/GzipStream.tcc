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

template<char Level>
GzipDecompressionStream_<Level>::GzipDecompressionStream_ (void) {
	stream.zalloc = Z_NULL;
	stream.zfree  = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = 0;
	stream.next_in  = Z_NULL;
	if (inflateInit(&stream) != Z_OK)
		throw;
	buffer = new char[CHUNK];
}

template<char Level>
GzipDecompressionStream_<Level>::~GzipDecompressionStream_ (void) {
	inflateEnd(&stream);
	delete[] buffer;
}

template<char Level>
void GzipDecompressionStream_<Level>::decompress (void *source, size_t sz, std::vector<char> &result) {
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
