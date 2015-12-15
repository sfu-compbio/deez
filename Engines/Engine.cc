#include "Engine.h"

size_t decompressArray (shared_ptr<DecompressionStream> d, uint8_t* &in, Array<uint8_t> &out) 
{
	size_t in_sz = *(size_t*)in; in += sizeof(size_t);
	size_t de_sz = *(size_t*)in; in += sizeof(size_t);
	
	out.resize(de_sz);
	size_t sz = 0;
	if (in_sz) 
		sz = d->decompress(in, in_sz, out, 0);
	assert(sz == de_sz);
	in += in_sz;
	return sz;
}