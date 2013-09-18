#include "PairedEnd.h"
using namespace std;

PairedEndCompressor::PairedEndCompressor (int blockSize):
	GenericCompressor<uint8_t, GzipCompressionStream<6> >(blockSize * 13) // 13 - szof PE 
{
}

PairedEndCompressor::~PairedEndCompressor (void) {
}

void PairedEndCompressor::addRecord (const std::string &chr, size_t pos, int32_t len) {
	int cnt;
	uint8_t *p;

	p = (uint8_t*)&pos, cnt = sizeof(size_t);
	while (cnt--) this->records.add(*p++);
	p = (uint8_t*)&len, cnt = sizeof(int32_t);
	while (cnt--) this->records.add(*p++);

	map<string, char>::iterator c = chromosomes.find(chr);
	if (c != chromosomes.end())
		this->records.add(c->second);
	else {
		this->records.add(-1);
		cnt = 0;
		while (cnt != chr.size() + 1) this->records.add(chr[cnt++]);
		chromosomes[chr] = chromosomes.size();
	}
}


PairedEndDecompressor::PairedEndDecompressor (int blockSize):
	GenericDecompressor<uint8_t, GzipDecompressionStream>(blockSize * 13) // 13 - szof PE 
{
}

PairedEndDecompressor::~PairedEndDecompressor (void) {
}

PairedEndInfo PairedEndDecompressor::getRecord (void) {
	assert(hasRecord());

	PairedEndInfo pe;
	char chr;

	pe.pos = *(size_t*)(records.data() + recordCount), recordCount += sizeof(size_t);
	pe.tlen = *(int32_t*)(records.data() + recordCount), recordCount += sizeof(int32_t);
	chr = *(records.data() + recordCount), recordCount++;

	if (chr == -1) {
		string s = "";
		while (records.data()[recordCount])
			s += records.data()[recordCount], recordCount++;
		recordCount++;
		size_t idx = chromosomes.size();
		pe.chr = chromosomes[idx] = s;
	}
	else pe.chr = chromosomes[chr];

	return pe;
}
