#include "PairedEnd.h"
using namespace std;

//size_t _OK=0;
//size_t _NO=0;

PairedEndCompressor::PairedEndCompressor (int blockSize):
	GenericCompressor<PairedEndInfo, GzipCompressionStream<6> >(blockSize)
{
}

PairedEndCompressor::~PairedEndCompressor (void) {
//	LOG("OK~ %lu NO~ %lu\n",_OK,_NO);
}

void PairedEndCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	Array<uint8_t> buffer(k * (sizeof(size_t) + sizeof(int32_t) + 1), 1000);
	std::map<std::string, char> chromosomes;
	for (size_t i = 0; i < k; i++) {
	//	if (records[i].notvalid) {
	//		_NO++;
	//		int t = 1;
	//		buffer.add((uint8_t*)&t, sizeof(int32_t));
			
			uint32_t sz = records[i].pos;
			if (sz < (1 << 16)) {
				uint16_t sz16 = sz;
				assert(sz16>0);
				buffer.add((uint8_t*)&sz16, sizeof(uint16_t));
			}
			else {
				uint16_t sz16 = 0;
				buffer.add((uint8_t*)&sz16, sizeof(uint16_t));	
				buffer.add((uint8_t*)&sz, sizeof(uint32_t));	
			}
			buffer.add((uint8_t*)&records[i].tlen, sizeof(int32_t));
			
			map<string, char>::iterator c = chromosomes.find(records[i].chr);
			if (c != chromosomes.end())
				buffer.add(c->second);
			else {
				buffer.add(-1);
				buffer.add((uint8_t*)this->records[i].chr.c_str(), this->records[i].chr.size() + 1);
				char id = chromosomes.size();
				chromosomes[records[i].chr] = id;
			}
	//	}
	//	else {
	//		buffer.add((uint8_t*)&records[i].tlen, sizeof(int32_t));
	//	}
	}
	compressArray(stream, buffer, out, out_offset);
	this->records.remove_first_n(k);
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

	assert(recordCount == records.size());
	assert(in_size >= sizeof(size_t));
	
	Array<uint8_t> au;
	size_t s = decompressArray(stream, in, au);

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
	LOG("loaded %d",records.size());
	
	recordCount = 0;
}

