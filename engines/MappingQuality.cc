#include "MappingQuality.h"
using namespace std;

static const char *LOG_PREFIX = "<MQ>";

MappingQualityCompressor::MappingQualityCompressor (const string &filename, int blockSize) {
	string name1(filename + ".mq.dz");
	file = gzopen(name1.c_str(), "wb6");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
	records.reserve(blockSize);
}

MappingQualityCompressor::~MappingQualityCompressor (void) {
	gzclose(file);
}

void MappingQualityCompressor::addRecord (unsigned char mq) {
	records.push_back(mq);
}

void MappingQualityCompressor::outputRecords (void) {
	if (records.size())
		gzwrite(file, &records[0], sizeof(records[0]) * records.size());
	records.clear();
}

MappingQualityDecompressor::MappingQualityDecompressor (const string &filename, int bs)
	: blockSize (bs), recordCount (0) {
		string name1(filename + ".mq.dz");
		file = gzopen(name1.c_str(), "rb");
		if (file == Z_NULL)
			throw DZException("Cannot open the file %s", name1.c_str());	
		records.reserve(blockSize);
	}

MappingQualityDecompressor::~MappingQualityDecompressor (void) {
	gzclose(file);
}

unsigned char MappingQualityDecompressor::getRecord (void) {
	if (records.size() == recordCount) {
		records.clear();
		recordCount = 0;
		importRecords();
	}
	return records[recordCount++];
}

void MappingQualityDecompressor::importRecords (void) {
	records.resize(blockSize);
	size_t size = gzread(file, &records[0], blockSize);
	if (size < blockSize)
		records.resize(size);

	LOG("%d mapping qualities are loaded", records.size());
}

