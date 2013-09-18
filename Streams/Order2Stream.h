#ifndef AC2Stream_H
#define AC2Stream_H

#include <vector>
#include "../Common.h"
#include "Order0Stream.h"	

class AC2CompressionStream: public CompressionStream, public DecompressionStream {
	AC0CompressionStream mod[128 * 128];

	uint8_t q1, q2;
	uint32_t context;

public:
	AC2CompressionStream (void) {
		q1 = q2 = 0;
		context = 0;
	}

private:
	void encode (uint8_t q, AC &ac) {
		assert(context<128*128);
		mod[context].encode(q, ac);

		context  = q1 << 7;
		context += q;
		q2 = q1; q1 = q;
	}

	uint8_t decode (AC &ac) {
		assert(context<128*128);
		uint8_t q = mod[context].decode(ac);
		
		context  = q1 << 7;
		context += q;
		q2 = q1; q1 = q;
		
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
		//return 
	}
};

typedef AC2CompressionStream AC2DecompressionStream;

#endif // AC2Stream_H