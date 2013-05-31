#include "PairedEnd.h"
using namespace std;

static const char *LOG_PREFIX = "<PE>";

PairedEndCompressor::PairedEndCompressor (const string &filename, int blockSize) {
	string name1(filename + ".pe.dz");
	file = gzopen(name1.c_str(), "wb6");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
	records.reserve(blockSize);
}

PairedEndCompressor::~PairedEndCompressor (void) {
	gzclose(file);
}

void PairedEndCompressor::addRecord (int chr, int pos, int tlen) {
    records.push_back(chr);
	records.push_back(pos);
    records.push_back(tlen);
    /*
    if (tlen != 0) {
	    records.push_back(tlen > 0 ? tlen : ((1 << 30) | -tlen));
	}
	else {
		records.push_back((1 << 31) | chr);
	    records.push_back(pos);
	}*/
}

void PairedEndCompressor::outputRecords (void) {
	if (records.size())
		gzwrite(file, &records[0], sizeof(records[0]) * records.size());
	records.clear();
}

PairedEndDecompressor::PairedEndDecompressor (const string &filename, int bs)
	: blockSize (bs), recordCount (0) {
	string name1(filename + ".pe.dz");
	file = gzopen(name1.c_str(), "rb");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());	
	records.reserve(blockSize);
}

PairedEndDecompressor::~PairedEndDecompressor (void) {
	gzclose(file);
}

int PairedEndDecompressor::getRecord (void) {
	if (records.size() == recordCount) {
		records.clear();
		recordCount = 0;
		importRecords();
	}
	return records[recordCount++];
}

void PairedEndDecompressor::importRecords (void) {
	records.resize(blockSize);
	size_t size = gzread(file, &records[0], blockSize * sizeof(int));
	if (size < blockSize)
		records.resize(size / sizeof(int));

	LOG("%d paired end data are loaded", records.size());
}

