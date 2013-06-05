#include <string.h>

#include "MappingOperation.h"
#include "../Streams/GzipStream.h"
using namespace std;

MappingOperationCompressor::MappingOperationCompressor (int blockSize):    
	lastLoc(0),
	lastRef(-1)
{
	stream = new GzipCompressionStream<6>();
	stitchStream = new GzipCompressionStream<6>();
	
	records.reserve(blockSize);
	corrections.reserve(blockSize);
}

MappingOperationCompressor::~MappingOperationCompressor (void) {
	delete stitchStream;
	delete stream;
}

void MappingOperationCompressor::addRecord (uint32_t tmpLoc, short tmpRef) {
	if (tmpRef != lastRef) {
		Tuple t;
		t.first = lastLoc + 1;
		t.second = tmpLoc;
		t.ref = tmpRef; 

		corrections.push_back(t);
		records.push_back(1);

		lastLoc = tmpLoc;
		lastRef = tmpRef;
	}
	else {
		if (tmpLoc - lastLoc > 250) {
			Tuple t;
			t.first = lastLoc + 1;
			t.second = tmpLoc;
			t.ref = tmpRef;

			corrections.push_back(t);
			records.push_back(1);

			lastLoc++;
		}
		else {
			records.push_back(tmpLoc - lastLoc);
		}
		lastLoc = tmpLoc;
	}
}

void MappingOperationCompressor::outputRecords (vector<char> &output) {
	if (records.size()) {
		output.clear();		
		
		LOG("output %d corrections", corrections.size());
		LOG("output %d records", records.size());

		if (corrections.size())
			stitchStream->compress(&corrections[0], corrections.size() * sizeof(corrections[0]), output);
		size_t i = output.size();
		output.insert(output.begin(), (char*)&i, (char*)&i + sizeof(size_t));
		corrections.erase(corrections.begin(), corrections.end());

		stream->compress(&records[0], records.size() * sizeof(records[0]), output);
		records.erase(records.begin(), records.end());
	}
}

MappingOperationDecompressor::MappingOperationDecompressor (int blockSize): 
	recordCount(0),
	correctionCount(0),
	lastLoc(0),
	lastRef(-1)
{
	stream = new GzipDecompressionStream();
	stitchStream = new GzipDecompressionStream();

	records.reserve(blockSize);
}

MappingOperationDecompressor::~MappingOperationDecompressor (void) {
	delete stitchStream;
	delete stream;
}

bool MappingOperationDecompressor::hasRecord (void) {
	return recordCount < records.size();
}

Locs MappingOperationDecompressor::getRecord (void) {
	assert(hasRecord());
	return records[recordCount++];
}

void MappingOperationDecompressor::importRecords (const vector<char> &input) {
	if (input.size() < sizeof(size_t)) return;
	
	size_t i = *(size_t*)(&input[0]);
	
	vector<char> c;

	if (i) {
		stitchStream->decompress((void*)(&input[0] + sizeof(size_t)), i, c);
		assert(c.size() % sizeof(Tuple) == 0);
		assert(corrections.size() == 0);
		corrections.resize(c.size() / sizeof(Tuple));
		memcpy(&corrections[0], &c[0], c.size());
		LOG("%d corrections are loaded", c.size() / sizeof(Tuple));
	}
			
	c.clear();
	stream->decompress((void*)(&input[0] + sizeof(size_t) + i), input.size() - sizeof(size_t) - i, c);
	assert(c.size() % sizeof(uint8_t) == 0);
	vector<uint8_t> rc(c.size() / sizeof(uint8_t));
	memcpy(&rc[0], &c[0], c.size());

	for (int i = 0; i < rc.size(); i++) {
		Locs n;
		if (correctionCount < corrections.size() && corrections[correctionCount].first == lastLoc + rc[i]) {
			n.loc = corrections[correctionCount].second;
			n.ref = corrections[correctionCount].ref;
			lastLoc = corrections[correctionCount].second;
			lastRef = corrections[correctionCount].ref;
			
			correctionCount++;
		}
		else {
			lastLoc += rc[i];
			n.loc = lastLoc;
			n.ref = lastRef;
		}
		records.push_back(n);
	}
	corrections.erase(corrections.begin(), corrections.begin() + correctionCount);
	correctionCount = 0;
	records.erase(records.begin(), records.begin() + recordCount);
	recordCount = 0;
	LOG("%d locations are loaded", rc.size());
}
