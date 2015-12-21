#include "ArithmeticCoder.h"

ArithmeticCoder::ArithmeticCoder (void) {
	rc_FFNum = rc_LowH = rc_Cache = rc_Carry = 0;
	Low = 0;
	Range = 0x7FFFFFFFFFFFFFFFll;
	Code = 0;
}

ArithmeticCoder::~ArithmeticCoder (void) {
}

inline void ArithmeticCoder::initEncode (Array<uint8_t> *o) {
	dataO = o;
}

inline void ArithmeticCoder::initDecode (uint8_t *o, size_t osz) {
	dataI = o;
	getbit(), getbit(), getbit();
}

inline int ArithmeticCoder::setbit (void) {
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

inline void ArithmeticCoder::getbit (void) {
	rc_Cache = *(uint32_t*)dataI; dataI += sizeof(uint32_t);
	Code = (Code << 32) + rc_Carry + (rc_Cache >> 1);
	rc_Carry = rc_Cache << 31;
}

inline int ArithmeticCoder::encode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq) {
	Range /= totFreq;
	Low += Range * cumFreq;
	Range *= freq;
	if (Range < 0x80000000) {
		Range <<= 32;
		return setbit();
	}
	return 0;
}

inline uint32_t ArithmeticCoder::getFreq (uint32_t totFreq) {
	return Code / (Range / totFreq);
}

//uint32_t symbol = model.find(count);
inline void ArithmeticCoder::decode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq) {	
	Range /= totFreq;
	Code -= Range * cumFreq;
	Range *= freq;
	if (Range < 0x80000000) {
		Range <<= 32;
		getbit();
	}
}

inline void ArithmeticCoder::flush (void) {
	Low++;
	setbit();
	setbit();
	setbit();
}