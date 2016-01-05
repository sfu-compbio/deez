#include "PairedEnd.h"
#include <unordered_map>
using namespace std;

PairedEndDecompressor::PairedEndDecompressor (int blockSize):
	GenericDecompressor<PairedEndInfo, GzipDecompressionStream>(blockSize)
{
	streams.resize(PairedEndCompressor::Fields::ENUM_COUNT);
	if (optBzip) {
		for (int i = 0; i < streams.size(); i++)
			streams[i] = make_shared<BzipDecompressionStream>();	
	} else {
		for (int i = 0; i < streams.size(); i++)
			streams[i] = make_shared<GzipDecompressionStream>();
	}
}

PairedEndInfo &PairedEndDecompressor::getRecord (size_t i, size_t opos, size_t ospan, bool reverse) 
{
	ZAMAN_START(PairedEndGet);

	PairedEndInfo &pe = records[i];
	//LOG("%d %d %d -- %d %d %d", records[i].bit, records[i].tlen, records[i].diff, opos, ospan, reverse);

	if (pe.bit >= PairedEndInfo::Bits::OK) { // otherwise fixed by Decompress main loop
		if ((pe.bit == PairedEndInfo::OK && reverse) || (pe.bit == PairedEndInfo::LESS_THAN_0)) {
			pe.tlen = -pe.tlen;
		}
		if (pe.tlen > 0) {
			pe.pos = opos + pe.tlen - ospan - pe.diff;
		} else {
			pe.pos = opos + pe.tlen + ospan - pe.diff;
		}	
	}

	ZAMAN_END(PairedEndGet);
	return pe;
}

void PairedEndDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) 
		return;
	assert(in_size >= sizeof(size_t));

	ZAMAN_START(PairedEndImport);
	
	Array<uint8_t> buffer, chromosomes, tlens, tlenBits, pointers;
	size_t s = decompressArray(streams[PairedEndCompressor::Fields::TLENBIT], in, tlenBits);
	decompressArray(streams[PairedEndCompressor::Fields::DIFF], in, buffer);
	decompressArray(streams[PairedEndCompressor::Fields::CHROMOSOME], in, chromosomes);
	decompressArray(streams[PairedEndCompressor::Fields::TLEN], in, tlens);

	uint8_t *chrs = chromosomes.data();
	uint8_t *tls = tlens.data();
	uint8_t *buf = buffer.data();
	uint8_t *ptr = pointers.data();

	PairedEndInfo pe;
	size_t vi = 0;
	records.resize(0);
	unordered_map<char, std::string> chromosomeIndex;
	char chr;
	for (size_t i = 0; i < s; i++) {
		pe.bit = tlenBits.data()[i];
		if (pe.bit >= PairedEndInfo::Bits::OK) {
			pe.diff = *(int32_t*)buf, buf += sizeof(int32_t);
			pe.tlen =  *(uint16_t*)tls, tls += sizeof(uint16_t);
			if (!pe.tlen) 
				pe.tlen = *(uint32_t*)tls, tls += sizeof(uint32_t);
			pe.tlen--;
		} 

		chr = *chrs++;
		if (chr == -1) {
			string sx = "";
			while (*chrs)
				sx += *chrs++;
			chrs++;
			chr = chromosomeIndex.size();
			pe.chr = chromosomeIndex[chr] = sx;
		} else {
			pe.chr = chromosomeIndex[chr];
		}	
		records.add(pe);
	}
	
	ZAMAN_END(PairedEndImport);
}

