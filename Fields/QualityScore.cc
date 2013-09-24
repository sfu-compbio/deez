#include "QualityScore.h"
using namespace std;

QualityScoreCompressor::QualityScoreCompressor (int blockSize):
	StringCompressor<AC2CompressionStream>(blockSize) 
{
}

QualityScoreCompressor::~QualityScoreCompressor (void) {
}

//void QualityScoreCompressor::addRecord (string qual, int flag) {
void QualityScoreCompressor::addRecord (string qual, int flag) {
	size_t sz = qual.size();
	if (flag & 0x10) for (size_t j = 0; j < sz / 2; j++)
		swap(qual[j], qual[sz - j - 1]);
	if (sz >= 2) {
		sz -= 2;
		while (sz && qual[sz] == qual[qual.size() - 1])
			sz--;
		sz += 2;
	}
	records.add(string(qual, 0, sz));
}


QualityScoreDecompressor::QualityScoreDecompressor (int blockSize):
	StringDecompressor<AC2DecompressionStream>(blockSize) 
{
}

QualityScoreDecompressor::~QualityScoreDecompressor (void) {
}

string QualityScoreDecompressor::getRecord (size_t seq_len, int flag) {
	assert(hasRecord());
	
	string s = string(StringDecompressor<AC2DecompressionStream>::getRecord());
    char c = s[s.size() - 1];
    while (s.size() < seq_len)
		s += c;
    if (flag & 0x10) for (size_t i = 0; i < seq_len / 2; i++)
		swap(s[i], s[seq_len - i - 1]);
    return s;
}
