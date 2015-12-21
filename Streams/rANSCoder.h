#ifndef rANSCoder_H
#define rANSCoder_H

// Based on rans_byte.h from https://github.com/rygorous/ryg_rans 
// Simple byte-aligned rANS encoder/decoder - public domain - Fabian 'ryg' Giesen 2014

#include <vector>
#include <zlib.h>
#include "../Common.h"

struct rANSCoder {
	static const uint32_t SCALE_BITS = 12;
	static const uint32_t SCALE = (1u << SCALE_BITS);
	static const uint32_t RANS_BYTE_L = 1u << 23;

	struct Stat {
		uint32_t x_max;     // (Exclusive) upper bound of pre-normalization interval
    	uint32_t rcp_freq;  // Fixed-point reciprocal frequency
		uint32_t bias;      // Bias
		uint16_t cmpl_freq; // Complement of frequency: (1 << scale_bits) - freq
		uint16_t rcp_shift; // Reciprocal shift
		uint32_t freq, start;
		
		Stat(void): 
			start(0), freq(0) 
		{
		}
		
		void setUpRans(void)
		{
			assert(start < SCALE);
			assert(start + freq < SCALE);

			x_max = ((RANS_BYTE_L >> SCALE_BITS) << 8) * freq;
			cmpl_freq = SCALE - freq;

			if (freq < 2) {
				rcp_freq = ~0u;
				rcp_shift = 0;
				bias = start + SCALE - 1;
			} else {
				uint32_t shift = 0;
				while (freq > (1u << shift))
					shift++;
				rcp_freq = ((1ull << (shift + 31)) + freq - 1) / freq;
				rcp_shift = shift - 1;
				bias = start;
			}
			rcp_shift += 32;
		}

		static inline void setUpFreqs(vector<vector<Stat>> &stats, uint32_t limit, uint32_t AS) 
		{
			assert(SCALE_BITS <= 16);

			for (size_t ctx = 0; ctx < limit; ctx++) {
				for (size_t i = 0; i < AS; i++)
					stats[ctx][i + 1].start = stats[ctx][i].start + stats[ctx][i].freq;
				assert(SCALE > AS);
		        while (stats[ctx][AS].start >= SCALE) {
		        	for (size_t i = 0; i < AS; i++) {
		                if (stats[ctx][i].freq) {
		                	stats[ctx][i].freq = ((uint64_t)SCALE * stats[ctx][i].freq) / double(stats[ctx][AS].start+1);
		                    if (!stats[ctx][i].freq) stats[ctx][i].freq++;
		                }
		                stats[ctx][i + 1].start = stats[ctx][i].start + stats[ctx][i].freq;
		            }
		        }
				for (size_t i = 0; i < AS; i++) {
					if (stats[ctx][i].freq) {
						assert(stats[ctx][i + 1].start > stats[ctx][i].start);
					} else {
						assert(stats[ctx][i + 1].start == stats[ctx][i].start);
					}
					stats[ctx][i].setUpRans();
				}
			}
		}
	};

	uint32_t state;
	Array<uint8_t> *out;
	uint8_t *in;

	rANSCoder(void):
		state(0), in(0), out(0)
	{
	}

	void initEncode(Array<uint8_t> *o)
	{
		out = o;
		state = RANS_BYTE_L;
	}

	void encode (const Stat &sym) 
	{
		while (state >= sym.x_max) 
			out->add(state & 0xff), state >>= 8;
		uint32_t q = (uint64_t(state) * sym.rcp_freq) >> sym.rcp_shift;
		state += sym.bias + q * sym.cmpl_freq;
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
};
	
#endif // rANSCoder_H
