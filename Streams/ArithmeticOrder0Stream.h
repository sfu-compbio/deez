#ifndef ArithmeticOrder0CompressionStream_H
#define ArithmeticOrder0CompressionStream_H

#include <vector>
#include <algorithm>
#include <functional>
#include "../Common.h"
#include "Stream.h"	
#include "ArithmeticCoder.h"	
using namespace std;

template<int AS>
class ArithmeticOrder2CompressionStream;

template<int AS> 
class ArithmeticOrder0CompressionStream: public CompressionStream, public DecompressionStream 
{
	static const int RescaleFactor = 32;
	static const int64_t SUM_LIMIT = 1ll << 15;

public:
	struct Stat 
	{
		uint16_t freq;
		uint8_t  sym;
		
		bool operator> (const Stat &s) const { 
			if (freq == s.freq) return sym > s.sym;
			return freq > s.freq; 
		}
	} stats[AS];
	int64_t sum;

public:
	size_t encoded;
	
public:
	ArithmeticOrder0CompressionStream (void) 
	{
		for (int i = 0; i < AS; i++)
			stats[i].sym = i, stats[i].freq = 1;
		sum = AS; 
		encoded = 0;
	}

protected:
	void rescale (void) 
	{
		sum = (stats[0].freq -= (stats[0].freq >> 1));
    	for (int i = 1; i < AS; i++) {
        	sum += (stats[i].freq -= (stats[i].freq >> 1));
        	int j = i - 1;
        	while (j && stats[i].freq > stats[j].freq) j--;
        	swap(stats[i], stats[j + 1]);
        }
	}

public:
	void encode (uint8_t c, ArithmeticCoder *ac) 
	{
		assert(c < AS);
		uint32_t l = 0, i;
		for (i = 0; i < AS; i++)
			if (stats[i].sym == c) break;
			else l += stats[i].freq;
		
		encoded += ac->encode(l, stats[i].freq, sum);
		sum++; 
		stats[i].freq++;

		if (i && sum % RescaleFactor == 0) {
			int j = i - 1;
			while (j && stats[i].freq > stats[j].freq) j--;
			swap(stats[i], stats[j + 1]);
		}

		if (sum > SUM_LIMIT)
			rescale();
	}

	uint8_t decode (ArithmeticCoder *ac) 
	{
		uint64_t cnt = ac->getFreq(sum);

		int i;
		uint32_t hi = 0;
		for (i = 0; i < AS; i++) {
			hi += stats[i].freq;
			if (cnt < hi) break;
		}
		assert(i!=AS);

		uint8_t sym = stats[i].sym;
		ac->decode(hi - stats[i].freq, stats[i].freq, sum);
		stats[i].freq++; sum++;
		
		if (i && sum % RescaleFactor == 0) {
			int j = i - 1;
			while (j && stats[i].freq > stats[j].freq) j--;
			swap(stats[i], stats[j + 1]);
		}

		if (sum > SUM_LIMIT)
			rescale();

		return sym;
	}

public:
	size_t compress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{
		if (source_sz == 0) return 0;
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

	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{
		size_t num = *((size_t*)source);
		if (!num) return 0;
		ArithmeticCoder ac;
		ac.initDecode(source + sizeof(size_t), source_sz);
		dest.resize(dest_offset + num);
		for (size_t i = 0; i < num; i++) 
			*(dest.data() + dest_offset + i) = decode(&ac);
		return num;
	}

	void getCurrentState (Array<uint8_t> &ou) 
	{
		for (int i = 0; i < AS; i++) {
			ou.add(stats[i].sym);
			ou.add((uint8_t*)&stats[i].freq, sizeof(stats[i].freq));
		}
	}

	void setCurrentState (uint8_t *in, size_t sz) 
	{
		sum = 0;
		for (int i = 0; i < AS; i++) {
			uint8_t sym = *in++;
			stats[i].sym = sym;
			stats[i].freq = *(uint16_t*)in; in += sizeof(stats[i].freq);
			sum += stats[i].freq;
		}	
	}
};

#define ArithmeticOrder0DecompressionStream ArithmeticOrder0CompressionStream

#endif // ArithmeticOrder0CompressionStream_H