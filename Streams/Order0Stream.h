#ifndef AC0Stream_H
#define AC0Stream_H

#include <vector>
#include <algorithm>
#include "../Common.h"
#include "ArithmeticStream.h"	
using namespace std;

class AC2CompressionStream;

class AC0CompressionStream: public CompressionStream, public DecompressionStream {
	struct Stat {
		int32_t sym, freq;
	} stats[256];
	int64_t sum;

public:
	AC0CompressionStream (void) {
		for (int i = 0; i < 256; i++)
			stats[i].sym = i, stats[i].freq = 1;
		sum = 256;
	}

protected:
	void rescale (void) {
		sum = 1 + (stats[0].freq -= (stats[0].freq >> 1));
    	for (int i = 1; i < 256; i++) {
        	sum += (stats[i].freq -= (stats[i].freq >> 1));
        	int j = i - 1;
        	while (j && stats[i].freq > stats[j].freq) j--;
        	swap(stats[i], stats[j + 1]);
        }
	}

	void encode (uint8_t c, AC &ac) {
		uint32_t l = 0, i;
		for (i = 0; i < 256; i++)
			if (stats[i].sym == c) break;
			else l += stats[i].freq;
		
		ac.encode(l, stats[i].freq, sum);
		sum++; 
		stats[i].freq++;

		if (i) {
			int j = i - 1;
			while (j && stats[i].freq > stats[j].freq) j--;
			swap(stats[i], stats[j + 1]);
		}

		if (sum > (1ll << 31))
			rescale();
	}

	uint8_t decode (AC &ac) {
		uint64_t cnt = ac.getFreq(sum);

		int i;
		uint32_t hi = 0;
		for (i = 0; i < 256; i++) {
			hi += stats[i].freq;
			if (cnt < hi) break;
		}
		assert(i!=256);

		uint8_t sym = stats[i].sym;
		ac.decode(hi - stats[i].freq, stats[i].freq, sum);
		stats[i].freq++; sum++;

		if (i) {
			int j = i - 1;
			while (j && stats[i].freq > stats[j].freq) j--;
			swap(stats[i], stats[j + 1]);
		}

		if (sum > (1ll << 31))
			rescale();

		return sym;
	}

public:
	size_t compress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) {
		// ac keeps appending to the array.
		// thus, just resize dest
		if (source_sz == 0) return 0;
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
		if (!num) return 0;
		AC ac;
		ac.initDecode(source + sizeof(size_t));
		dest.resize(dest_offset + num);
		for (size_t i = 0; i < num; i++) {
			*(dest.data() + i) = decode(ac);
		}
		return num;
		//return 
	}

	friend class AC2CompressionStream;
};

typedef AC0CompressionStream AC0DecompressionStream;

#endif // AC0Stream_H