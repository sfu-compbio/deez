#include "PairedEnd.h"
using namespace std;

//size_t _OK=0;
//size_t _NO=0;

PairedEndCompressor::PairedEndCompressor (int blockSize):
	GenericCompressor<PairedEndInfo, GzipCompressionStream<6> >(blockSize)
{
	tlenStream[0] = new GzipCompressionStream<6>();
	tlenStream[1] = new GzipCompressionStream<6>();
	tlenBitStream = new GzipCompressionStream<6>();
	chromosomeStream  = new GzipCompressionStream<6>();
}

PairedEndCompressor::~PairedEndCompressor (void) 
{
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
	Array<uint8_t> tlens2(k * 4, MB);
	Array<uint8_t> tlenBits(k, MB);
	Array<uint8_t> chromosomes(k + 10 * KB, MB);

	static size_t 
		_valid = 0, 
		_total = 0;
	
	std::map<std::string, char> chromosomeIndex;
	int prevTlen = 0;
	for (size_t i = 0; i < k; i++) {	
		int32_t tlen = abs(records[i].tlen)+1;
		if (tlen < (1 << 16)) {
			uint16_t sz16 = tlen;
			assert(sz16>0);
			tlens.add((uint8_t*)&sz16, sizeof(uint16_t));
		}
		else {
			uint16_t sz16 = 0;
			tlens.add((uint8_t*)&sz16, sizeof(uint16_t));	
			tlens.add((uint8_t*)&tlen, sizeof(uint32_t));	
		}
		
		bool reverse = records[i].flag & 0x10;
		if ((records[i].tlen < 0) != reverse)
			tlenBits.add(records[i].tlen < 0);
		int diff;
		records[i].tlen;
		if (records[i].tlen > 0) // replace opos with size
			diff = records[i].opos + records[i].tlen - records[i].ospan - records[i].pos;
		else 
			diff = records[i].opos + records[i].ospan + records[i].tlen - records[i].pos;
		if (!diff) { // eg. assume perfect mapping
			buffer.add(0);
			_valid++;
		} else {
			buffer.add(diff);
		}
		_total++;
		
		map<string, char>::iterator c = chromosomeIndex.find(records[i].chr);
		if (c != chromosomeIndex.end())
			chromosomes.add(c->second);
		else {
			chromosomes.add(-1);
			chromosomes.add((uint8_t*)this->records[i].chr.c_str(), this->records[i].chr.size() + 1);
			char id = chromosomeIndex.size();
			chromosomeIndex[records[i].chr] = id;
		}
	}
	compressArray(stream, buffer, out, out_offset);
	compressArray(chromosomeStream, chromosomes, out, out_offset);
	compressArray(tlenStream[0], tlens, out, out_offset);
	compressArray(tlenBitStream, tlenBits, out, out_offset);
	this->records.remove_first_n(k);
	//LOGN("\n%'d %'d\n",_valid,_total);
}

PairedEndDecompressor::PairedEndDecompressor (int blockSize):
	GenericDecompressor<PairedEndInfo, GzipDecompressionStream>(blockSize)
{
}

PairedEndDecompressor::~PairedEndDecompressor (void) {
}

const PairedEndInfo &PairedEndDecompressor::getRecord (const string &mc, size_t mpos) {
	assert(hasRecord());
	PairedEndInfo &p = records.data()[recordCount++];
	if (mc == p.chr) {
		if (p.tlen <= 0)
			p.pos += mpos;
		else
			p.pos = mpos - p.pos;
	}
	p.pos--;
	return p;
}

void PairedEndDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) 
		return;

	//assert(recordCount == records.size());
	assert(in_size >= sizeof(size_t));
	
	Array<uint8_t> au;
	size_t s = decompressArray(stream, in, au);
__debug_fwrite(au.data(), 1, au.size(), ____debug_file[__DC++]);

	PairedEndInfo pe;
	size_t vi = 0;
	records.resize(0);
	std::map<char, std::string> chromosomes;
	char chr;
	for (size_t i = 0; i < s; ) {
	//	pe.notvalid = 0;
	//	if (pe.tlen == 1) {
	//		pe.notvalid = 1;
	//		pe.tlen = *(int32_t*)(au.data() + i), i += sizeof(int32_t);
			pe.pos = *(uint16_t*)(au.data() + i), i += sizeof(uint16_t);
			if (!pe.pos) 
				pe.pos = *(uint32_t*)(au.data() + i), i += sizeof(uint32_t);
			pe.tlen = *(int32_t*)(au.data() + i), i += sizeof(int32_t);
			chr = *(au.data() + i), i++;
			if (chr == -1) {
				string sx = "";
				while (au.data()[i])
					sx += au.data()[i++];
				i++;
				chr = chromosomes.size();
			//	DEBUG("%d->%s",chr,sx.c_str());
				pe.chr = chromosomes[chr] = sx;
			}
			else pe.chr = chromosomes[chr];
		
	//	}	
		records.add(pe);
	}
	//LOG("loaded %d",records.size());
	
	recordCount = 0;
}

