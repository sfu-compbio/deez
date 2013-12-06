#ifndef ArithmeticStream_H
#define ArithmeticStream_H

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
	int encode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq);
	void decode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq);
	uint32_t getFreq (uint32_t);
	void flush (void);


	void initEncode (Array<uint8_t> *o);
	void initDecode (uint8_t *o);

public:
	AC ();
	~AC (void);
};
	
#endif
