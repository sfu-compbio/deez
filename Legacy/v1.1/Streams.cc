#include "Streams.h"

namespace Legacy {
namespace v11 {

GzipDecompressionStream::GzipDecompressionStream (void) {
}

GzipDecompressionStream::~GzipDecompressionStream (void) {
}

size_t GzipDecompressionStream::decompress (uint8_t *source, size_t source_sz, 
	Array<uint8_t> &dest, size_t dest_offset) {
	size_t sz = dest.size() - dest_offset;

	int c = uncompress(dest.data() + dest_offset, &sz, source, source_sz);

	if (c == Z_OK) 
		return sz;
	else 
		throw DZException("zlib decompression error %d", c);
}

AC::AC (void) {
	rc_FFNum = rc_LowH = rc_Cache = rc_Carry = 0;
	Low = 0;
	Range = 0x7FFFFFFFFFFFFFFFll;
	Code = 0;
}

AC::~AC (void) {
}

void AC::initEncode (Array<uint8_t> *o) {
	dataO = o;
}

void AC::initDecode (uint8_t *o, size_t osz) {
	dataI = o;
	getbit(), getbit(), getbit();
}

int AC::setbit (void) {
	int w = 0;

	rc_Carry = (Low >> 63) & 1;
	Low &= 0x7FFFFFFFFFFFFFFFll;
	rc_LowH = Low >> 31;
	if (rc_Carry == 1 || rc_LowH < 0xFFFFFFFF) {
		uint32_t i = rc_Cache + rc_Carry;
		dataO->add((uint8_t*)&i, 4), w += 4;
		for (; rc_FFNum > 0; rc_FFNum--) {
			i = 0xFFFFFFFF + rc_Carry;
			dataO->add((uint8_t*)&i, 4), w += 4;
		}
		rc_Cache = rc_LowH;
	}
	else rc_FFNum++;
	Low = (Low & 0x7FFFFFFF) << 32;

	return w;
}

void AC::getbit (void) {
	rc_Cache = *(uint32_t*)dataI; dataI += sizeof(uint32_t);
	Code = (Code << 32) + rc_Carry + (rc_Cache >> 1);
	rc_Carry = rc_Cache << 31;
}

int AC::encode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq) {
	Range /= totFreq;
	Low += Range * cumFreq;
	Range *= freq;
	if (Range < 0x80000000) {
		Range <<= 32;
		return setbit();
	}
	return 0;
}

uint32_t AC::getFreq (uint32_t totFreq) {
	return Code / (Range / totFreq);
}

void AC::decode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq) {	
	Range /= totFreq;
	Code -= Range * cumFreq;
	Range *= freq;
	if (Range < 0x80000000) {
		Range <<= 32;
		getbit();
	}
}

void AC::flush (void) {
	Low++;
	setbit();
	setbit();
	setbit();
}

};
};