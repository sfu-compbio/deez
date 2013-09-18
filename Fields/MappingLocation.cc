#include <string>
#include <string.h>

#include "MappingLocation.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
using namespace std;

MappingLocationCompressor::MappingLocationCompressor (int blockSize):    
	records(blockSize), corrections(blockSize),
	lastLoc(0), lastRef("")
{
	stream = new AC0CompressionStream();
	stitchStream = new GzipCompressionStream<6>();
}

MappingLocationCompressor::~MappingLocationCompressor (void) {
	delete stitchStream;
	delete stream;
}

void MappingLocationCompressor::addRecord (uint32_t tmpLoc, const string &tmpRef) {
	if (tmpRef != lastRef || tmpLoc - lastLoc >= 254) {
		records.add(255);
		corrections.add(tmpLoc);
		lastRef = tmpRef;
	}
	else {
		records.add(tmpLoc - lastLoc);
	}
	lastLoc = tmpLoc;
}

void MappingLocationCompressor::outputRecords (Array<uint8_t> &output) {
	if (!records.size()) {
		output.resize(0);
		return;
	}

	// attach size ..... ~~~~~~~

	size_t s = stitchStream->compress((uint8_t*)corrections.data(), 
		corrections.size() * sizeof(uint32_t), output, sizeof(size_t));
	memcpy(output.data(), &s, sizeof(size_t)); // copy size
	
	size_t sn = stream->compress(records.data(), records.size(), 
		output, s + 2 * sizeof(size_t));
	memcpy(output.data() + s + sizeof(size_t), &sn, sizeof(size_t));

	output.resize(sn + s + 2 * sizeof(size_t));

	//DEBUG("%lu %lu >> comp %lu %lu --- %lu\n", corrections.size(), records.size(), s, sn, output.size());

	corrections.resize(0);
	records.resize(0);
}

MappingLocationDecompressor::MappingLocationDecompressor (int blockSize): 
	recordCount(0),
	correctionCount(0),
	lastLoc(0), 
	records(blockSize), corrections(blockSize)
{
	stream = new AC0DecompressionStream();
	stitchStream = new GzipDecompressionStream();
}

MappingLocationDecompressor::~MappingLocationDecompressor (void) {
	delete stitchStream;
	delete stream;
}

bool MappingLocationDecompressor::hasRecord (void) {
	return recordCount < records.size();
}

uint32_t MappingLocationDecompressor::getRecord (void) {
	assert(hasRecord());
	if (records.data()[recordCount++] == 255) 
		return lastLoc = corrections.data()[correctionCount++];
	else
		return lastLoc += records.data()[recordCount - 1];
}

void MappingLocationDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) return;

	// move to the front ....   erase first recordCount
	if (recordCount < records.size())
		memmove(records.data(), records.data() + recordCount, 
			records.size() - recordCount);
	records.resize(records.size() - recordCount);
	if (correctionCount < corrections.size())
		memmove(corrections.data(), corrections.data() + correctionCount, 
			(corrections.size() - correctionCount) * sizeof(uint32_t));
	corrections.resize(corrections.size() - correctionCount);

	// decompress
	Array<uint8_t> au;
	au.resize( 51000000 );

	size_t in1 = *((size_t*)in);
	size_t s = stitchStream->decompress(in + sizeof(size_t), in1, au, 0);
	corrections.add((uint32_t*)au.data(), s / sizeof(uint32_t));	
	
	int in2 = *((size_t*)(in + in1 + sizeof(size_t)));
	s = stream->decompress(in + in1 + 2 * sizeof(size_t), in2, au, 0);
	records.add(au.data(), s);
	
	recordCount = 0;
	correctionCount = 0;
}