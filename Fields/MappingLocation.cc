#include <string>
#include <string.h>

#include "MappingLocation.h"

MappingLocationCompressor::MappingLocationCompressor (int blockSize):    
	GenericCompressor<size_t, AC0CompressionStream>(blockSize),
	lastLoc(0)
{
	stitchStream = new GzipCompressionStream<6>();
}

MappingLocationCompressor::~MappingLocationCompressor (void) {
	delete stitchStream;
}

void MappingLocationCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	Array<uint8_t> buffer;
	Array<uint32_t> corrections;	

	for (size_t i = 0; i < k; i++) {
		if (this->records[i] - lastLoc >= 254)
			buffer.add(255), corrections.add(this->records[i]);
		else
			buffer.add(this->records[i] - lastLoc);
		lastLoc = this->records[i];
	}

	size_t s = stitchStream->compress((uint8_t*)corrections.data(), 
		corrections.size() * sizeof(uint32_t), out, out_offset + sizeof(size_t));
	memcpy(out.data() + out_offset, &s, sizeof(size_t));
	
	size_t sn = stream->compress(buffer.data(), buffer.size(), 
		out, out_offset + s + 2 * sizeof(size_t));
	memcpy(out.data() + out_offset + s + sizeof(size_t), &sn, sizeof(size_t));
	out.resize(out_offset + sn + s + 2 * sizeof(size_t));
	//// 
	this->records.remove_first_n(k);
}

MappingLocationDecompressor::MappingLocationDecompressor (int blockSize): 
	GenericDecompressor<size_t, AC0DecompressionStream>(blockSize),
	lastLoc(0)
{
	stitchStream = new GzipDecompressionStream();
}

MappingLocationDecompressor::~MappingLocationDecompressor (void) {
	delete stitchStream;
}

void MappingLocationDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) return;

	assert(recordCount == records.size());

	// decompress
	Array<uint8_t> au, au2;
	au.resize( 51000000 );
	au2.resize( 51000000 );

	size_t in1 = *((size_t*)in);
	size_t s = stitchStream->decompress(in + sizeof(size_t), in1, au, 0);
	
	int in2 = *((size_t*)(in + in1 + sizeof(size_t)));
	size_t s2 = stream->decompress(in + in1 + 2 * sizeof(size_t), in2, au2, 0);

	size_t j = 0;
	for (size_t i = 0; i < s2; i++)
		if (au2.data()[i] == 255)
			records.add(lastLoc = j++[(uint32_t*)au.data()]);
		else
			records.add(lastLoc += au2.data()[i]);

	recordCount = 0;
}