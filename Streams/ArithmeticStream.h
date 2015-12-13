#ifndef ArithmeticStream_H
#define ArithmeticStream_H

// Based on cld-c.inc from aridemo6.rar package available at http://compression.ru/sh/aridemo6.rar
// Dword-oriented Rangecoder by Eugene D. Shelwien -- Plain C++

#include <vector>
#include <zlib.h>
#include "../Common.h"

class AC {
	uint64_t Range;
	uint64_t Low;
	uint64_t Code;

	uint32_t rc_FFNum;		// number of all-F dwords between Cache and Low
	uint32_t rc_LowH;		// high 32bits of Low
	uint32_t rc_Cache;		// cached low dword
	uint32_t rc_Carry;		// Low carry flag

	uint8_t *dataI;
	Array<uint8_t> *dataO;

private:
	int setbit (void);
	void getbit (void);

public:
	virtual int encode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq);
	virtual void decode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq);
	virtual uint32_t getFreq (uint32_t);
	virtual void flush (void);


	virtual void initEncode (Array<uint8_t> *o);
	virtual void initDecode (uint8_t *o, size_t osz);

public:
	AC ();
	~AC (void);
};
	
#endif
