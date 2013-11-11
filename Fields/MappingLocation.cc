#include <string>
#include <string.h>

#include "MappingLocation.h"

MappingLocationCompressor::MappingLocationCompressor (int blockSize):    
	GenericCompressor<size_t, AC0CompressionStream<256> >(blockSize)
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

	Array<uint8_t> buffer(k);
	Array<uint32_t> corrections(k);	

	uint32_t lastLoc = 0;
	for (size_t i = 0; i < k; i++) {
		if (this->records[i] - lastLoc >= 254)
			buffer.add(255), corrections.add(this->records[i]);
		else
			buffer.add(this->records[i] - lastLoc);
		lastLoc = this->records[i];
	}

	size_t s = 0;
	if (corrections.size()) s = stitchStream->compress((uint8_t*)corrections.data(), 
		corrections.size() * sizeof(uint32_t), out, out_offset +  2 * sizeof(size_t));
	// size of compressed
	*(size_t*)(out.data() + out_offset) = s;
	// size of uncompressed
	*(size_t*)(out.data() + out_offset + sizeof(size_t)) = corrections.size() * sizeof(uint32_t);
	
	out_offset += s + 2 * sizeof(size_t);
	out.resize(out_offset);

	s = 0;
	if (buffer.size()) s = stream->compress(buffer.data(), buffer.size(), 
		out, out_offset + 2 * sizeof(size_t));
	*(size_t*)(out.data() + out_offset) = s;
	// size of uncompressed
	*(size_t*)(out.data() + out_offset + sizeof(size_t)) = buffer.size();

	out_offset += s + 2 * sizeof(size_t);
	
	out.resize(out_offset);
	//// 
	this->records.remove_first_n(k);
}

MappingLocationDecompressor::MappingLocationDecompressor (int blockSize): 
	GenericDecompressor<size_t, AC0DecompressionStream<256> >(blockSize)
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

	size_t in1 = *(size_t*)in; in += sizeof(size_t);
	size_t de1 = *(size_t*)in; in += sizeof(size_t);
	Array<uint8_t> au1;
	au1.resize(de1);
	size_t s1 = 0;
	if (in1) s1 = stitchStream->decompress(in, in1, au1, 0);
	assert(s1 == de1);
	in += in1;

	size_t in2 = *(size_t*)in; in += sizeof(size_t);
	size_t de2 = *(size_t*)in; in += sizeof(size_t);
	Array<uint8_t> au2;
	au2.resize(de2);
	size_t s2 = 0;
	if (in2) s2 = stream->decompress(in, in2, au2, 0);
	assert(s2 == de2);
	in += in2;

	size_t j = 0;
	records.resize(0);
	uint32_t lastLoc = 0;
	for (size_t i = 0; i < s2; i++)
		if (au2.data()[i] == 255)
			records.add(lastLoc = j++[(uint32_t*)au1.data()]);
		else
			records.add(lastLoc += au2.data()[i]);

	recordCount = 0;
}