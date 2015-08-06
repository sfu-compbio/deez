#include "rANSStream.h"

#define BYTEL (1u << 23)
#define SCALE 12

/*
File run/9827_2#49.dz already exists. Overwriting it.
Compressing run/9827_2#49.bam to run/9827_2#49.dz ...
Using quality mode default
   Chr *      100.00% [130]5]]
Written 56,463,236 lines
[Fixes 14629411 Replace 10282588]sequence: 24911999
[Nucleotides 32020455 Unknown 0 Operations 19209200 Oplens 37524673 Locations 47374748 Stitch 2572749]editOp: 138701825
[Index 13697786 Content 208558060]readName: 222255846
mapFlag: 21601073
mapQual: 7619451
quality: 2569991524
pairedEnd: 249037213
optField: 336879045

real	11m21.597s
user	17m32.070s
sys	0m34.363s


File run/9827_2#49.dz already exists. Overwriting it.
Compressing run/9827_2#49.bam to run/9827_2#49.dz ...
Using quality mode default
   Chr *      100.00% [130]5]]
Written 56,463,236 lines
[Fixes 14629411 Replace 10282588]sequence: 24911999
[Nucleotides 32020455 Unknown 0 Operations 19209200 Oplens 37524673 Locations 50399902 Stitch 2572749]editOp: 141726979
[Index 13697786 Content 208558060]readName: 222255846
mapFlag: 21601073
mapQual: 7619451
quality: 2897591257
pairedEnd: 249037213
optField: 336879045

real    11m6.875s
user    17m25.992s
sys     0m34.953s
*/

rANS::rANS (void) 
{
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
	//	if(!StateMax)printf(".%u_%u.[%u_%u_%u]",State,StateMax, cumFreq,freq,totFreq);
	}

	//printf("%d\n",dataO->size()/1024);

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