#ifndef BStream_H
#define BStream_H

#include <vector>
#include "../Common.h"
#include "Order0Stream.h"	

template<int AS>
class BCompressionStream: public CompressionStream {
	static const int MOD_SZ = (1<<12)*16*16;
	AC0CompressionStream<AS> mod[MOD_SZ];

	uint8_t q1, q2;
	uint32_t context;
	int delta;
	int pos;

public:
	BCompressionStream (void) {
		q1 = q2 = 0;
		context = 0;
		pos = 0;
		delta = 5;
	}

private:
	void encode (uint8_t q, AC &ac) {
		assert(q < AS);
		assert(q < (1 <<6));
		assert(context<MOD_SZ);
		mod[context].encode(q, ac);

		context  = q1 * AS;
		context += q;
		q1 = q;

	#define QBITS 12
		context  = ((max(q1,q2)<<6) + q) & ((1<<QBITS)-1);
		context += (q1==q2)<<QBITS;
		delta   += (q1>q)*(q1-q);
		context += min(7,delta>>3) << (QBITS+1);
		context += (min(pos++ +15,127)&(15<<3))<<(QBITS+1);

		q2 = q1; q1 = q;
		
		if(q==0) delta = 5, pos = 0, context = 0, q1 = q2 = 0;
	}

public:
	size_t compress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) {
		// ac keeps appending to the array.
		// thus, just resize dest
		dest.resize(dest_offset + sizeof(size_t));
		memcpy(dest.data() + dest_offset, &source_sz, sizeof(size_t));
		AC ac;
		ac.initEncode(&dest);
		for (size_t i = 0; i < source_sz; i++)
			encode(source[i], ac);
		ac.flush();
		return dest.size() - dest_offset;
		//return 
	}

	void getCurrentState (Array<uint8_t> &ou) {
	}
};

#endif // AC2Stream_H
