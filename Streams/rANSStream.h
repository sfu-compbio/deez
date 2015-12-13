#ifndef rANSStream_H
#define rANSStream_H

// Based on rans_byte.h from https://github.com/rygorous/ryg_rans 
// Simple byte-aligned rANS encoder/decoder - public domain - Fabian 'ryg' Giesen 2014

#include <vector>
#include <zlib.h>
#include "../Common.h"
#include "ArithmeticStream.h"

class rANS : public AC {
	uint32_t State;

	uint8_t *dataI;
	Array<uint8_t> *dataO;

public:
	int encode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq);
	void decode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq);
	uint32_t getFreq (uint32_t totFreq);
	void flush (void);


	void initEncode (Array<uint8_t> *o);
	void initDecode (uint8_t *o, size_t osz);

public:
	rANS (void);
	~rANS (void);
};
	
#endif
