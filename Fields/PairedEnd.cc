#include "PairedEnd.h"
#include <unordered_map>
using namespace std;

enum PairedEndFields {
	DIFF,
	CHROMOSOME,
	TLEN,
	TLENBIT,
	ENUM_COUNT
};

PairedEndCompressor::PairedEndCompressor (int blockSize):
	GenericCompressor<PairedEndInfo, GzipCompressionStream<6> >(blockSize)
{
	for (int i = 0; i < PairedEndFields::ENUM_COUNT; i++)
		streams.push_back(new GzipCompressionStream<6>());
}

PairedEndCompressor::~PairedEndCompressor (void) 
{
	for (int i = 0; i < streams.size(); i++)
		delete streams[i];
}

size_t PairedEndCompressor::compressedSize(void) 
{ 
	int res = 0;
	for (int i = 0; i < streams.size(); i++) 
		res += streams[i]->getCount();
	return res;
}

void PairedEndCompressor::printDetails(void) 
{
	vector<const char*> s { "DIFF", "CHROMOSOME", "TLEN", "TBIT" };
	for (int i = 0; i < streams.size(); i++) 
		LOG("  %-10s: %'20lu ", s[i], streams[i]->getCount());
}

void PairedEndCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	Array<int32_t> buffer(k, MB);
	Array<uint8_t> tlens(k * 4, MB);
	Array<uint8_t> tlenBits(k, MB);
	Array<uint8_t> chromosomes(k + 10 * KB, MB);

	static size_t _valid = 0, _total = 0;
	unordered_map<string, char> chromosomeIndex;
	for (size_t i = 0; i < k; i++) {	
		int32_t tlen = abs(records[i].tlen) + 1;
		if (tlen < (1 << 16)) {
			uint16_t sz16 = tlen;
			assert(sz16 > 0);
			tlens.add((uint8_t*)&sz16, sizeof(uint16_t));
		} else {
			uint16_t sz16 = 0;
			tlens.add((uint8_t*)&sz16, sizeof(uint16_t));	
			tlens.add((uint8_t*)&tlen, sizeof(uint32_t));	
		}
		tlenBits.add(records[i].bit);
		buffer.add(records[i].diff);
		if (!records[i].diff) _valid++; _total++;
		
		auto c = chromosomeIndex.find(records[i].chr);
		if (c != chromosomeIndex.end()) {
			chromosomes.add(c->second);
		} else {
			chromosomes.add(-1);
			chromosomes.add((uint8_t*)this->records[i].chr.c_str(), this->records[i].chr.size() + 1);
			char id = chromosomeIndex.size();
			chromosomeIndex[records[i].chr] = id;
		}
	}

	compressArray(streams[DIFF], buffer, out, out_offset);
	compressArray(streams[CHROMOSOME], chromosomes, out, out_offset);
	compressArray(streams[TLEN], tlens, out, out_offset);
	compressArray(streams[TLENBIT], tlenBits, out, out_offset);
	this->records.remove_first_n(k);
	//LOGN("\n%'d %'d\n",_valid,_total);
}

PairedEndDecompressor::PairedEndDecompressor (int blockSize):
	GenericDecompressor<PairedEndInfo, GzipDecompressionStream>(blockSize)
{
	for (int i = 0; i < PairedEndFields::ENUM_COUNT; i++)
		streams.push_back(new GzipDecompressionStream());
}

PairedEndDecompressor::~PairedEndDecompressor (void) 
{	
	for (int i = 0; i < streams.size(); i++)
		delete streams[i];
}

const PairedEndInfo &PairedEndDecompressor::getRecord (size_t opos, size_t ospan, bool reverse) 
{
	assert(hasRecord());
	PairedEndInfo &pe = records.data()[recordCount++];
	if ((!pe.bit && reverse) || (pe.bit == 2)) {
		pe.tlen = -pe.tlen;
	}
	if (pe.tlen > 0) {
		pe.pos = opos + pe.tlen - ospan - pe.diff;
	} else {
		pe.pos = opos + pe.tlen + ospan - pe.diff;
	}
	//LOG("%d %d %d %d %s", pe.tlen, pe.pos, pe.diff, pe.bit, pe.chr.c_str());
	return pe;
}

void PairedEndDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) 
		return;
	assert(in_size >= sizeof(size_t));
	
	Array<uint8_t> buffer, chromosomes, tlens, tlenBits;
	size_t s = decompressArray(streams[DIFF], in, buffer);
	decompressArray(streams[CHROMOSOME], in, chromosomes);
	decompressArray(streams[TLEN], in, tlens);
	decompressArray(streams[TLENBIT], in, tlenBits);

	uint8_t *chrs = chromosomes.data();
	uint8_t *tls = tlens.data();
	uint8_t *tlb = tlenBits.data();

	PairedEndInfo pe;
	size_t vi = 0;
	records.resize(0);
	unordered_map<char, std::string> chromosomeIndex;
	char chr;
	for (size_t i = 0; i < s; ) {
		pe.diff = *(int32_t*)(buffer.data() + i), i += sizeof(int32_t);
		
		pe.tlen =  *(uint16_t*)tls, tls += sizeof(uint16_t);
		if (!pe.tlen) 
			pe.tlen = *(uint32_t*)tls, tls += sizeof(uint32_t);
		pe.tlen--;
		pe.bit = *tlb++;

		chr = *chrs++;
		if (chr == -1) {
			string sx = "";
			while (*chrs)
				sx += *chrs++;
			chrs++;
			chr = chromosomeIndex.size();
			pe.chr = chromosomeIndex[chr] = sx;
		}
		else pe.chr = chromosomeIndex[chr];
		
	//	}	
		records.add(pe);
	}
	//LOG("loaded %d",records.size());
	
	recordCount = 0;
}

