#include "EditOperation.h"
using namespace std;

static const char *LOG_PREFIX = "<EO>";
static const int  RECORD_AVG  = 50;

/*** Compression ***/

EditOperationCompressor::EditOperationCompressor (const string &filename, int blockSize) {
	string name1(filename + ".eo.dz");
	file = gzopen(name1.c_str(), "wb6");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
	buffer.reserve(blockSize * RECORD_AVG);
}

EditOperationCompressor::~EditOperationCompressor (void) {
	gzclose(file);
}

void EditOperationCompressor::addRecord (const string &rec) {
	buffer += rec + '\0';
}

void EditOperationCompressor::outputRecords (void) {
	if (buffer.size() > 0)
		gzwrite(file, buffer.c_str(), buffer.size());
	buffer.clear();
}

/*** Decompression ***/

EditOperationDecompressor::EditOperationDecompressor (const string &filename, int bs)
	: blockSize (bs), recordCount (0) {
	string name1(filename + ".eo.dz");
	file = gzopen(name1.c_str(), "rb");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
}

EditOperationDecompressor::~ EditOperationDecompressor (void) {
	gzclose(file);
}

string EditOperationDecompressor::getRecord (void) {
	if (records.size() == recordCount) {
		records.clear();
		recordCount = 0;
		importRecords();
	}
	return records[recordCount++];
}

void EditOperationDecompressor::importRecords (void) {
	char *c = new char[blockSize * RECORD_AVG];
	size_t size = gzread(file, c, blockSize * RECORD_AVG);

	string tmp = "";
	size_t pos = 0;
	while (pos < size) {
		if (c[pos] != 0)
			tmp += c[pos];
		else {
			records.push_back(tmp);
			tmp="";
		}
		pos++;
	}
	if (pos == size && c[pos - 1] != 0) {
		unsigned char t = c[pos - 1];
		while (t) {
			gzread(file, &t, 1);
			tmp += t;
		}
		records.push_back(tmp);
	}

	LOG("%d edit operations are loaded", records.size());
	delete[] c;
}
