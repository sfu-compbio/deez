#include "QualityScore.h"
using namespace std;

static const char *LOG_PREFIX = "<QS>";
static const int  RECORD_AVG 	= 100;

QualityScoreCompressor::QualityScoreCompressor (const string &filename, int blockSize) {
	string name1(filename + ".qs.dz");
	file = gzopen(name1.c_str(), "wb6");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
	buffer.reserve(blockSize * RECORD_AVG);
}

QualityScoreCompressor::~QualityScoreCompressor (void) {
	gzclose(file);
}

void QualityScoreCompressor::addRecord (const string &rec) {
	buffer += rec + '\0';
}

void QualityScoreCompressor::outputRecords (void) {
	if (buffer.size() > 0)
		gzwrite(file, buffer.c_str(), buffer.size());
	buffer.clear();
}

QualityScoreDecompressor::QualityScoreDecompressor (const string &filename, int bs)
	: blockSize (bs), recordCount (0) {
		string name1(filename + ".qs.dz");
		file = gzopen(name1.c_str(), "rb");
		if (file == Z_NULL)
			throw DZException("Cannot open the file %s", name1.c_str());
	}

QualityScoreDecompressor::~QualityScoreDecompressor (void) {
	gzclose(file);
}

string QualityScoreDecompressor::getRecord (void) {
	if (records.size() == recordCount) {
		records.clear();
		recordCount = 0;
		importRecords();
	}
	return records[recordCount++];
}

void QualityScoreDecompressor::importRecords (void) {
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
		while (t) { // border case
			gzread(file, &t, 1);
			tmp += t;
		}
		records.push_back(tmp);
	}

	LOG("%d quality scores are loaded", records.size());
	delete[] c;
}

