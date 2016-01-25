#ifndef SAMCompStream_H
#define SAMCompStream_H

// This is a C++ rewrite of sam_comp's context model
// developed by J. Bonfield and M. Mahoney
// http://sourceforge.net/projects/samcomp/

#include <vector>
#include "../Common.h"
#include "ArithmeticOrder0Stream.h"	

template<int AS>
class SAMCompStream: public CompressionStream, public DecompressionStream {
	static const int QBITS  = 12;
	static const int MOD_SZ = (1 << QBITS) * 16 * 16;
	ArithmeticOrder0CompressionStream<AS> mod[MOD_SZ];

	uint8_t q1, q2;
	uint32_t context;
	int delta;
	int pos;
	bool hadSought;

public:
	SAMCompStream (void) 
	{
		q1 = q2 = 0;
		context = 0;
		pos = 0;
		delta = 5;
		hadSought = false;
	}

private:
	void encode (uint8_t q, ArithmeticCoder *ac) 
	{
		assert(q < AS);
		assert(q < (1 << 6));
		assert(context < MOD_SZ);
		mod[context].encode(q, ac);

		context  = ((max(q1, q2) << 6) + q) & ((1 << QBITS)-1);
		context += (q1 == q2) << QBITS;
		delta   += (q1 > q) * (q1 - q);
		context += min(7, delta >> 3) << (QBITS + 1);
		context += (min(pos++ + 15, 127) & (15 << 3)) << (QBITS + 1);
		q2 = q1, q1 = q;
		
		if (!q) 
			delta = 5, pos = 0, context = 0, q1 = q2 = 0;
	}

	uint8_t decode (ArithmeticCoder *ac) 
	{
		assert(context < MOD_SZ);
		uint8_t q = mod[context].decode(ac);
		assert(q < AS);
		assert(q < (1 << 6));
		
		context  = ((max(q1, q2) << 6) + q) & ((1 << QBITS)-1);
		context += (q1 == q2) << QBITS;
		delta   += (q1 > q) * (q1 - q);
		context += min(7, delta >> 3) << (QBITS + 1);
		context += (min(pos++ + 15, 127) & (15 << 3)) << (QBITS + 1);
		q2 = q1, q1 = q;
		
		if (!q) 
			delta = 5, pos = 0, context = 0, q1 = q2 = 0;
		
		return q;
	}

public:
	size_t compress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) 
	{
		dest.resize(dest_offset + sizeof(size_t));
		memcpy(dest.data() + dest_offset, &source_sz, sizeof(size_t));
		ArithmeticCoder ac;
		ac.initEncode(&dest);
		for (size_t i = 0; i < source_sz; i++)
			encode(source[i], &ac);
		ac.flush();
		this->compressedCount += dest.size() - dest_offset;
		return dest.size() - dest_offset;
	}

	size_t decompress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) 
	{
		size_t num = *((size_t*)source);
		ArithmeticCoder ac;
		ac.initDecode(source + sizeof(size_t), source_sz);
		dest.resize(dest_offset + num);

		// fill with 33s. zero terminators are not added. 
		// the qualityscore class will take care of that.
		if (hadSought) for (size_t i = 0; i < num; i++)
			*(dest.data() + dest_offset + i) = 33;
		else for (size_t i = 0; i < num; i++) 
			*(dest.data() + dest_offset + i) = decode(&ac);
		return num;
	}

	void getCurrentState (Array<uint8_t> &ou) 
	{
		ou.add(0);
	}

	void setCurrentState (uint8_t *in, size_t sz) 
	{
		hadSought = true;
	}
};

#endif // SAMCompStream_H
