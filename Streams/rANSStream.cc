#include "rANSStream.h"

#define BYTEL (1u << 23)
#define SCALE 12

rANS::rANS (void) 
{
	LOG("Using rANS stream");
}

rANS::~rANS (void) 
{
}

void rANS::initEncode (Array<uint8_t> *o) 
{
	dataO = o;
	State = BYTEL;
}

void rANS::initDecode (uint8_t *o, size_t osz) 
{
	dataI = o + osz;
	State = 0;
	State |= (*--dataI) <<  0;
	State |= (*--dataI) <<  8;
	State |= (*--dataI) << 16;
	State |= (*--dataI) << 24;
}

int rANS::encode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq) 
{
	uint32_t StateMax = ((BYTEL >> SCALE) << 8) * freq;
	while (State >= StateMax) {
		dataO->add(State & 0xff), State >>= 8; 
		if(!StateMax)printf(".%u_%u.[%u_%u_%u]",State,StateMax, cumFreq,freq,totFreq);
	}

	State = ((State / freq) << SCALE) + cumFreq + (State % freq);
	return 0;
}

uint32_t rANS::getFreq (uint32_t totFreq) 
{
	return State & ((1u << SCALE) - 1);
}

void rANS::decode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq) 
{	
	uint32_t mask = (1u << SCALE) - 1;
	State = freq * (State >> SCALE) + (State & mask) - cumFreq;
	
	while (State < BYTEL) 
		State = (State << 8) | *--dataI;
}

void rANS::flush (void) 
{
	dataO->add(State >> 24);
	dataO->add(State >> 16);
	dataO->add(State >>  8);
	dataO->add(State >>  0);
}