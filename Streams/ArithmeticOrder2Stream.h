#ifndef ArithmeticOrder2CompressionStream_H
#define ArithmeticOrder2CompressionStream_H

#include <vector>
#include "../Common.h"
#include "ArithmeticOrder0Stream.h"	

template<int AS>
class ArithmeticOrder2CompressionStream: public CompressionStream, public DecompressionStream {
	static const int MOD_SZ = AS * AS;
	ArithmeticOrder0CompressionStream<AS> mod[MOD_SZ];

	uint8_t loq, hiq;
	uint8_t q1;
	uint32_t context;

public:
	ArithmeticOrder2CompressionStream (void) 
	{
		q1 = loq = hiq = 0;
		context = 0;
	}

private:
	void encode (uint8_t q, ArithmeticCoder *ac) 
	{
		assert(q < AS);
		assert(context < MOD_SZ);
		mod[context].encode(q, ac);
		
		// squeeze index
		loq = min(loq, q);
		hiq = max(hiq, q);

		context  = q1 * AS;
		context += q;
		q1 = q;
	}

	uint8_t decode (ArithmeticCoder *ac) 
	{
		assert(context < MOD_SZ);
		uint8_t q = mod[context].decode(ac);

		context  = q1 * AS;
		context += q;
		q1 = q;

		loq = min(loq, q);
		hiq = max(hiq, q);
		
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
		for (size_t i = 0; i < num; i++) {
			*(dest.data() + dest_offset + i) = decode(&ac);
		}
		return num;
	}

	void getCurrentState (Array<uint8_t> &ou) 
	{
		ou.add(loq);
		ou.add(hiq);
		for (int i = loq; i <= hiq; i++) 
			for (int j = loq; j <= hiq; j++)
				mod[i * AS + j].getCurrentState(ou);
		ou.add(q1);
		ou.add((uint8_t*)&context, sizeof(uint32_t));
	}

	void setCurrentState (uint8_t *in, size_t sz) 
	{
		loq = *in++;
		hiq = *in++;

		for (int i = loq; i <= hiq; i++) 
			for (int j = loq; j <= hiq; j++) {
				size_t szx = AS * (1 + sizeof(uint16_t));
				mod[i * AS + j].setCurrentState(in, szx);
				in += szx;
				//eha += szx;
			}
		q1 = *in++;
		context = *(uint32_t*)in;
	}
};

#define ArithmeticOrder2DecompressionStream ArithmeticOrder2CompressionStream

#endif // ArithmeticOrder2CompressionStream_H
