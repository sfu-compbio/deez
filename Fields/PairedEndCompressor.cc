#include "PairedEnd.h"
#include <unordered_map>
using namespace std;

PairedEndCompressor::PairedEndCompressor (int blockSize):
	GenericCompressor<PairedEndInfo, GzipCompressionStream<6> >(blockSize)
{
	for (int i = 0; i < Fields::ENUM_COUNT; i++)
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
	vector<const char*> s { "Diff", "Chr", "Template", "Bits", "Pointer" };
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

	ZAMAN_START(Compress_PairedEnd);

	Array<int32_t> buffer(k, MB);
	Array<uint8_t> tlens(k * 4, MB);
	Array<uint8_t> tlenBits(k, MB);
	Array<uint8_t> chromosomes(k + 10 * KB, MB);

	Array<uint8_t> ptrs(k * 4, MB);

	static size_t _valid = 0, _total = 0;
	unordered_map<string, char> chromosomeIndex;
	for (size_t i = 0; i < k; i++) {
		tlenBits.add(records[i].bit);	
		
		if (records[i].bit == PairedEndInfo::Bits::LOOK_BACK) {
			int32_t tlen = records[i].tlen;
			assert(tlen > 0);
			if (tlen < (1 << 16)) {
				uint16_t sz16 = tlen;
				assert(sz16 > 0);
				ptrs.add((uint8_t*)&sz16, sizeof(uint16_t));
			} else {
				uint16_t sz16 = 0;
				ptrs.add((uint8_t*)&sz16, sizeof(uint16_t));	
				ptrs.add((uint8_t*)&tlen, sizeof(uint32_t));	
			}
		} else if (records[i].bit >= PairedEndInfo::Bits::OK) {
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
			buffer.add(records[i].diff);
			if (!records[i].diff) _valid++; _total++;
		}

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

	compressArray(streams[Fields::TLENBIT], tlenBits, out, out_offset);
	compressArray(streams[Fields::DIFF], buffer, out, out_offset);
	compressArray(streams[Fields::CHROMOSOME], chromosomes, out, out_offset);
	compressArray(streams[Fields::TLEN], tlens, out, out_offset);
	compressArray(streams[Fields::POINTER], ptrs, out, out_offset);
	this->records.remove_first_n(k);
	
	ZAMAN_END(Compress_PairedEnd);
}
