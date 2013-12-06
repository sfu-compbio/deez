#ifndef AC2Stream_H
#define AC2Stream_H

#include <vector>
#include "../Common.h"
#include "Order0Stream.h"	

template<int AS>
class AC2CompressionStream: public CompressionStream, public DecompressionStream {
	static const int MOD_SZ = AS*AS;
	AC0CompressionStream<AS> mod[MOD_SZ];

	uint8_t q1;
	uint32_t context;

public:
	AC2CompressionStream (void) {
		q1 = 0;
		context = 0;
	}

private:
	void encode (uint8_t q, AC &ac) {
		assert(q < AS);
		assert(context<MOD_SZ);
		mod[context].encode(q, ac);

		context  = q1 * AS;
		context += q;
		q1 = q;
	}

	uint8_t decode (AC &ac) {
		assert(context<AS*AS);
		uint8_t q = mod[context].decode(ac);
		
		context  = q1 * AS;
		context += q;
		q1 = q;
		
		return q;
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
		this->compressedCount += dest.size() - dest_offset;
		return dest.size() - dest_offset;
		//return 
	}

	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) {
		
		// ac keeps appending to the array.
		// thus, just resize dest

		size_t num = *((size_t*)source);
		AC ac;
		ac.initDecode(source + sizeof(size_t));
		dest.resize(dest_offset + num);
		for (size_t i = 0; i < num; i++) {
			*(dest.data() + i) = decode(ac);
		}
		return num;
	}

	void getCurrentState (Array<uint8_t> &ou) {
		for (int i = 0; i < AS * AS; i++) 
			mod[i].getCurrentState(ou);
		ou.add(q1);
		ou.add((uint8_t*)&context, sizeof(uint32_t));
		//printf("<<%d,%u>>\n",q1,context);
	}

	void setCurrentState (uint8_t *in, size_t sz) {
		for (int i = 0; i < AS * AS; i++) {
			// first dictates
			// uint8_t t = *in;
			size_t sz = AS * (1 + sizeof(uint16_t));
			mod[i].setCurrentState(in, sz);
			in += sz;
		}
		q1 = *in++;
		context = *(uint32_t*)in;
	}
};

#define AC2DecompressionStream AC2CompressionStream

#endif // AC2Stream_H
