#include "MappingFlag.h"
using namespace std;

static const char *LOG_PREFIX = "<MF>";

MappingFlagCompressor::MappingFlagCompressor (const string &filename, int blockSize) {
	string name1(filename + ".mf.dz");
	file = gzopen(name1.c_str(), "wb6");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
	records.reserve(blockSize);
}

MappingFlagCompressor::~MappingFlagCompressor (void) {
	gzclose(file);
}

void MappingFlagCompressor::addRecord (short rec) {
	records.push_back(rec);
}

void MappingFlagCompressor::outputRecords (void) {
	if (records.size())
		gzwrite(file, &records[0], sizeof(records[0]) * records.size());
	records.clear();
}

MappingFlagDecompressor::MappingFlagDecompressor (const string &filename, int bs)
	: blockSize (bs), recordCount (0) {
	string name1(filename + ".mf.dz");
	file = gzopen(name1.c_str(), "rb");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());	
	records.reserve(blockSize);
}

MappingFlagDecompressor::~MappingFlagDecompressor (void) {
	gzclose(file);
}

short MappingFlagDecompressor::getRecord (void) {
	if (records.size() == recordCount) {
		records.clear();
		recordCount = 0;
		importRecords();
	}
	return records[recordCount++];
}

void MappingFlagDecompressor::importRecords (void) {
	records.resize(blockSize);
	size_t size = gzread(file, &records[0], blockSize * sizeof(short));
	if (size < blockSize)
		records.resize(size / sizeof(short));

	LOG("%d mapping flags data are loaded", records.size());
}

