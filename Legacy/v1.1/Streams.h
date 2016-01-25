#pragma once

#include <zlib.h>
#include "Common.h"

namespace Legacy {
namespace v11 {

class DecompressionStream {
public:
	virtual ~DecompressionStream (void) {}

public:
	virtual size_t decompress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) = 0;//uint8_t *dest, size_t dest_sz) = 0;
	virtual void setCurrentState (uint8_t *source, size_t source_sz) = 0;
};

class AC {
	uint64_t Range;
	uint64_t Low;
	uint64_t Code;

	uint32_t rc_FFNum;		// number of all-F dwords between Cache and Low
	uint32_t rc_LowH;		// high 32bits of Low
	uint32_t rc_Cache;		// cached low dword
	uint32_t rc_Carry;		// Low carry flag

	uint8_t *dataI;
	Array<uint8_t> *dataO;

private:
	int setbit (void);
	void getbit (void);

public:
	virtual int encode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq);
	virtual void decode (uint32_t cumFreq, uint32_t freq, uint32_t totFreq);
	virtual uint32_t getFreq (uint32_t);
	virtual void flush (void);

	virtual void initEncode (Array<uint8_t> *o);
	virtual void initDecode (uint8_t *o, size_t osz);

public:
	AC ();
	~AC (void);
};

class GzipDecompressionStream: public DecompressionStream {
public:
	GzipDecompressionStream (void);
	~GzipDecompressionStream (void);

public:
	virtual size_t decompress (uint8_t *source, size_t source_sz, 
		Array<uint8_t> &dest, size_t dest_offset); 
	virtual void setCurrentState (uint8_t *source, size_t source_sz) {}
};

template<int AS> 
class AC0DecompressionStream: public DecompressionStream 
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
	AC0DecompressionStream (void) 
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
        	std::swap(stats[i], stats[j + 1]);
        }
	}

public:
	uint8_t decode (AC *ac) 
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
			std::swap(stats[i], stats[j + 1]);
		}

		if (sum > SUM_LIMIT)
			rescale();

		return sym;
	}

public:
	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{
		size_t num = *((size_t*)source);
		if (!num) return 0;
		AC *ac = new AC();
		ac->initDecode(source + sizeof(size_t), source_sz);
		dest.resize(dest_offset + num);
		for (size_t i = 0; i < num; i++) 
			*(dest.data() + dest_offset + i) = decode(ac);
		delete ac;
		return num;
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

template<int AS>
class AC2DecompressionStream: public DecompressionStream {
	static const int MOD_SZ = AS * AS;
	AC0DecompressionStream<AS> mod[MOD_SZ];

	uint8_t loq, hiq;
	uint8_t q1;
	uint32_t context;

public:
	AC2DecompressionStream (void) {
		q1 = loq = hiq = 0;
		context = 0;
	}

private:
	uint8_t decode (AC *ac) {
		assert(context < MOD_SZ);
		uint8_t q = mod[context].decode(ac);

		context  = q1 * AS;
		context += q;
		q1 = q;

		loq = std::min(loq, q);
		hiq = std::max(hiq, q);
		
		return q;
	}

public:
	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{	
		size_t num = *((size_t*)source);
		AC *ac = new AC();
		ac->initDecode(source + sizeof(size_t), source_sz);
		dest.resize(dest_offset + num);
		for (size_t i = 0; i < num; i++) {
			*(dest.data() + dest_offset + i) = decode(ac);
		}
		return num;
	}

	void setCurrentState (uint8_t *in, size_t sz) {
		loq = *in++;
		hiq = *in++;

		for (int i = loq; i <= hiq; i++) 
			for (int j = loq; j <= hiq; j++) {
				size_t szx = AS * (1 + sizeof(uint16_t));
				mod[i * AS + j].setCurrentState(in, szx);
				in += szx;
			}
		q1 = *in++;
		context = *(uint32_t*)in;
	}
};

template<int AS>
class SAMCompStream: public DecompressionStream {
	static const int QBITS  = 12;
	static const int MOD_SZ = (1 << QBITS) * 16 * 16;
	AC0DecompressionStream<AS> mod[MOD_SZ];

	uint8_t q1, q2;
	uint32_t context;
	int delta;
	int pos;
	bool hadSought;

public:
	SAMCompStream (void) {
		q1 = q2 = 0;
		context = 0;
		pos = 0;
		delta = 5;
		hadSought = false;
	}

private:
	uint8_t decode (AC *ac) {
		assert(context < MOD_SZ);
		uint8_t q = mod[context].decode(ac);
		assert(q < AS);
		assert(q < (1 << 6));
		
		context  = ((std::max(q1, q2) << 6) + q) & ((1 << QBITS)-1);
		context += (q1 == q2) << QBITS;
		delta   += (q1 > q) * (q1 - q);
		context += std::min(7, delta >> 3) << (QBITS + 1);
		context += (std::min(pos++ + 15, 127) & (15 << 3)) << (QBITS + 1);
		q2 = q1, q1 = q;
		
		if (!q) 
			delta = 5, pos = 0, context = 0, q1 = q2 = 0;
		
		return q;
	}

public:
	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{
		size_t num = *((size_t*)source);
		AC *ac = new AC();
		ac->initDecode(source + sizeof(size_t), source_sz);
		dest.resize(dest_offset + num);

		// fill with 33s. zero terminators are not added. 
		// the qualityscore class will take care of that.
		if (hadSought) for (size_t i = 0; i < num; i++)
			*(dest.data() + dest_offset + i) = 33;
		else for (size_t i = 0; i < num; i++) 
			*(dest.data() + dest_offset + i) = decode(ac);
		delete ac;
		return num;
	}

	void setCurrentState (uint8_t *in, size_t sz) {
		hadSought = true;
	}
};

struct ACTGStream {
	Array<uint8_t> seqvec, Nvec;
	int seqcnt, Ncnt;
	uint8_t seq, N;
	uint8_t *pseq, *pN;

	ACTGStream() {}
	ACTGStream(size_t cap, size_t ext):
		seqvec(cap, ext), Nvec(cap, ext) {}

	void initEncode (void) {
		seqcnt = 0, Ncnt = 0, seq = 0, N = 0;
	}

	void initDecode (void) {
		seqcnt = 6, pseq = seqvec.data();
		Ncnt = 7, pN = Nvec.data();
	}

	void add (char c) {
		assert(isupper(c));
		if (seqcnt == 4) 
			seqvec.add(seq), seqcnt = 0;
		if (Ncnt == 8)
			Nvec.add(N), Ncnt = 0;
		if (c == 'N') {
			seq <<= 2, seqcnt++;
			N <<= 1, N |= 1, Ncnt++;
			return;
		}
		if (c == 'A')
			N <<= 1, Ncnt++;
		seq <<= 2;
		seq |= "\0\0\001\0\0\0\002\0\0\0\0\0\0\0\0\0\0\0\0\003"[c - 'A'];
		seqcnt++;
	}

	void add (const char *str, size_t len)  {
		for (size_t i = 0; i < len; i++) 
			add(str[i]);
	}

	void flush (void) {
		while (seqcnt != 4) seq <<= 2, seqcnt++;
		seqvec.add(seq), seqcnt = 0;
		while (Ncnt != 8) N <<= 1, Ncnt++;
		Nvec.add(N), Ncnt = 0;
	}

	void get (std::string &out, size_t sz) {
		const char *DNA = "ACGT";
		const char *AN  = "AN";

		for (int i = 0; i < sz; i++) {
			char c = (*pseq >> seqcnt) & 3;
			if (!c) {
				out += AN[(*pN >> Ncnt) & 1];
				if (Ncnt == 0) Ncnt = 7, pN++;
				else Ncnt--;
			}
			else out += DNA[c];
			
			if (seqcnt == 0) seqcnt = 6, pseq++;
			else seqcnt -= 2;
		}
	}
};

}; 
};