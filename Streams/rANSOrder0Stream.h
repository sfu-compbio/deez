#ifndef rANS0Stream_H
#define rANS0Stream_H

#include <vector>
#include <algorithm>
#include <functional>
#include "../Common.h"
#include "Stream.h"	
#include "ArithmeticStream.h"	
#include "rANSStream.h"	
using namespace std;

// 46,773,780 3.6 O2 AC
// 51,110,320 2.8 O0 AC
// 51,154,303 1.0 O0 rANS

template<int AS> 
class rANSOrder0CompressionStream: public CompressionStream, public DecompressionStream 
{
	static const uint32_t SCALE_BITS = 14;
	static const uint32_t SCALE = (1u << SCALE_BITS);
	static const uint32_t RANS_BYTE_L = 1u << 23;

public:
	struct Stat;
	struct rANScoder {
		uint32_t state;
		Array<uint8_t> *out;
		uint8_t *in;

		void initEncode(Array<uint8_t> *o)
		{
			out = o;
			state = RANS_BYTE_L;
		}

		void encode (const Stat &sym) 
		{
			while (state >= sym.rans.x_max) {
				out->add(state & 0xff);
				state >>= 8;
			}
			uint32_t q = (uint64_t(state) * sym.rans.rcp_freq) >> sym.rans.rcp_shift;
			state += sym.rans.bias + q * sym.rans.cmpl_freq;
		}

		void flush () 
		{
			// LOG("Enc flush: %u %x%x%x%x", state, 
			// 	uint8_t(state&0xff),
			// 	uint8_t((state>>8)&0xff),
			// 	uint8_t((state>>16)&0xff),
			// 	uint8_t((state>>24)&0xff));
			REPEAT(4) out->add(state & 0xff), state >>= 8; 
		}

		/////

		void initDecode(uint8_t *i)
		{
			in = i;
			state = 0;
			REPEAT(4) state = (state << 8) | *in--;
			// LOG("Dec init: %u %x%x%x%x", state, 
			// 	uint8_t(state&0xff),
			// 	uint8_t((state>>8)&0xff),
			// 	uint8_t((state>>16)&0xff),
			// 	uint8_t((state>>24)&0xff));
		}

		void decode(const Stat &sym)
		{
			state = sym.freq * (state >> SCALE_BITS) + (state & (SCALE - 1)) - sym.start;
		}

		void renormalize()
		{
			while (state < RANS_BYTE_L) 
				state = (state << 8) | *in--;
		}

		uint32_t get()
		{
			return state & (SCALE - 1);
		}
	} coder;

	struct Stat {
		struct rANS {
	    	uint32_t x_max;     // (Exclusive) upper bound of pre-normalization interval
	    	uint32_t rcp_freq;  // Fixed-point reciprocal frequency
    		uint32_t bias;      // Bias
    		uint16_t cmpl_freq; // Complement of frequency: (1 << scale_bits) - freq
    		uint16_t rcp_shift; // Reciprocal shift
    	} rans;
		uint16_t freq, start;
		
		Stat() {}
		Stat(uint16_t start, uint16_t freq):
			start(start), freq(freq)
		{
			assert(start < SCALE);
			assert(start + freq < SCALE);

			rans.x_max = ((RANS_BYTE_L >> SCALE_BITS) << 8) * freq;
			rans.cmpl_freq = SCALE - freq;

			if (freq < 2) {
				rans.rcp_freq = ~0u;
				rans.rcp_shift = 0;
				rans.bias = start + SCALE - 1;
			} else {
				uint32_t shift = 0;
				while (freq > (1u << shift))
					shift++;
				rans.rcp_freq = ((1ull << (shift + 31)) + freq - 1) / freq;
				rans.rcp_shift = shift - 1;
				rans.bias = start;
			}
			rans.rcp_shift += 32;
		}
	} stats[AS + 1];


	void setUpFreqs(uint8_t *arr, size_t sz) 
	{
		assert(SCALE_BITS <= 16);

		array<uint64_t, AS + 1> freq{};
		array<uint64_t, AS + 1> start{};
		for (size_t i = 0; i < sz; i++)
			freq[arr[i]]++;
		for (size_t i = 0; i < AS; i++)
			start[i + 1] = start[i] + freq[i];

		assert(SCALE > AS);
        while (start[AS] >= SCALE) {
            for (size_t i = 0; i < AS; i++) {
                if (freq[i]) {
                    freq[i] = (SCALE * freq[i]) / start[AS];
                    if (!freq[i]) freq[i]++;
                }
                start[i + 1] = start[i] + freq[i];
            }
        }

		for (size_t i = 0; i < AS; i++) {
			if (freq[i]) {
				assert(start[i + 1] > start[i]);
			} else {
				assert(start[i + 1] == start[i]);
			}
			stats[i] = Stat(start[i], freq[i]);
		}
	}

public:
	size_t compress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{
		// ac keeps appending to the array.
		// thus, just resize dest
		if (source_sz == 0) return 0;
		dest.resize(dest_offset + sizeof(size_t));
		memcpy(dest.data() + dest_offset, &source_sz, sizeof(size_t));
		
		setUpFreqs(source, source_sz);
		getCurrentState(dest);
		size_t tdx = dest.size() - dest_offset - sizeof(size_t);

		coder.initEncode(&dest);
		for (ssize_t i = source_sz - 1; i >= 0; i--) 
			coder.encode(stats[source[i]]);
		coder.flush();
		this->compressedCount += (dest.size() - dest_offset) - tdx;
		return (dest.size() - dest_offset);
	}

	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{
		size_t num = *((size_t*)source);
		if (!num) return 0;

		source += sizeof(size_t);
		source_sz -= sizeof(size_t);
		setCurrentState(source, source_sz);
		source += sizeof(uint16_t) * AS;
		source_sz -= sizeof(uint16_t) * AS;

		vector<uint8_t> rev(stats[AS].start);
		for (size_t i = 0; i < AS; i++) 
			fill(rev.begin() + stats[i].start, rev.begin() + stats[i + 1].start, i);

		coder.initDecode(source + source_sz - 1);
		dest.resize(dest_offset + num);
		for (size_t i = 0; i < num; i++) {
			assert(coder.get() < stats[AS].start);
			size_t s = rev[coder.get()];
			*(dest.data() + dest_offset + i) = s;
			coder.decode(stats[s]);
			coder.renormalize();
		}
		assert(coder.in + 1 == source);
		return num;
	}

	void getCurrentState (Array<uint8_t> &ou) 
	{
		for (int i = 0; i < AS; i++) 
			ou.add((uint8_t*)&stats[i].freq, sizeof(uint16_t));
	}

	void setCurrentState (uint8_t *in, size_t sz) 
	{
		stats[0].start = 0;
		for (size_t i = 0; i < AS; i++) {
			stats[i].freq = *(uint16_t*)in; in += sizeof(uint16_t);
			stats[i + 1].start = stats[i].freq + stats[i].start;
		}
		assert(stats[AS].start < SCALE);
	}
};

#define rANSOrder0DecompressionStream rANSOrder0CompressionStream

#endif // rANS0Stream_H