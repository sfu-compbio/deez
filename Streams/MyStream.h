#ifndef MyStream_H
#define MyStream_H

#include <cmath>
#include <vector>
#include "../Common.h"
#include "Order0Stream.h"	

static float _mean[] = 
{ 65.00,64.00,65.00,68.00,68.00,68.00,68.00,68.00,70.00,70.00,70.00,70.00,70.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,71.00,70.00,71.00,71.00,70.00,70.00,70.00,70.00,70.00,70.00,70.00,70.00,70.00,70.00,70.00,70.00,70.00,69.00,66.00,67.00,68.00,65.00,67.00,68.00,68.00,69.00,69.00,69.00,69.00,69.00,69.00,69.00,68.00,68.00,68.00,68.00,68.00,68.00,67.00,67.00,67.00,67.00,66.00,66.00,66.00,65.00 };
static float _stdev[] =
{ 8.54,8.72,8.54,8.77,8.77,8.77,8.77,8.77,9.49,9.49,9.49,9.49,9.49,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.95,9.49,9.95,9.95,9.49,9.49,9.49,9.49,9.49,9.49,9.49,9.49,9.49,9.49,9.49,9.49,9.49,9.06,8.49,8.60,8.77,8.54,8.60,8.77,8.77,9.11,9.22,9.22,9.22,9.22,9.22,9.22,8.89,8.89,8.89,8.89,8.89,8.89,8.72,8.72,8.72,8.72,8.60,8.60,8.60,8.66 };

template<int AS>
class MyCompressionStream: public CompressionStream {
	static const int MOD_SZ = 200;
	AC0CompressionStream<AS> stdev[MOD_SZ];
	AC0CompressionStream<AS> plusminus;
	AC0CompressionStream<AS> other[MOD_SZ];
	uint32_t context;
	
	vector<int> mean;

public:
	MyCompressionStream (void) {
		context = 0;
		for (int i = 0; i < sizeof(_mean)/sizeof(float); i++)
			_mean[i] -= 32.0;
	}

	~MyCompressionStream() {
		size_t encoded = 0, enco = 0;
		for (int i = 0; i < MOD_SZ; i++)
			encoded += stdev[i].encoded,
			enco += other[i].encoded;
		LOG("Stats: stdev %lu", encoded);
		LOG("Stats: +-    %lu", plusminus.encoded);
		LOG("Stats: other %lu", enco);
	}

private:
	void encode (uint8_t q, AC &ac) {
		assert(context < MOD_SZ);

		if (q <= (int)(_mean[context] + _stdev[context] + 1) &&
			q >= (int)(_mean[context] - _stdev[context]))
		{
			stdev[context].encode(q, ac);
		}
		else 
		{
			stdev[context].encode(0, ac);
			other[context].encode(q, ac);
		}
		/*
		int x = q - mean[context];
		uint8_t sgn = x >= 0 ? 0 : 1;
		uint8_t val = abs(x);
		plusminus.encode(sgn, ac);
		mod[context].encode(val, ac);
		*/
		context++;
		if (q == 0) context = 0;
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
