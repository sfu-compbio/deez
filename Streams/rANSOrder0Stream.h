#ifndef rANSOrder0CompressionStream_H
#define rANSOrder0CompressionStream_H

#include <vector>
#include "../Common.h"
#include "../Engines/Engine.h"
#include "rANSCoder.h"	

template<int AS>
class rANSOrder0CompressionStream: public CompressionStream, public DecompressionStream {
public:
	inline uint8_t find(const vector<rANSCoder::Stat> &stat, size_t f) 
	{
		int l = 0, h = AS - 1, m;
		while (l <= h) {
			m = l + (h-l) / 2;
			if (f >= stat[m].start && f < stat[m+1].start)
				return m;
			else if (f < stat[m+1].start) 
				h = m - 1;
			else
				l = m + 1;
		}
		assert(0);
	}

	size_t compress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) 
	{
		dest.resize(dest_offset + sizeof(size_t));
		memcpy(dest.data() + dest_offset, &source_sz, sizeof(size_t));

		auto stats = vector<vector<rANSCoder::Stat>>(1, vector<rANSCoder::Stat>(AS + 1));
		for (ssize_t i = 0; i < source_sz; i++) 
			stats[0][source[i]].freq++;
		rANSCoder::Stat::setUpFreqs(stats, 1, AS);

		Array<uint8_t> freqData(0, MB);
		char fMin = -1, fMax;
		for (int i = 0; i < AS; i++)
			if (stats[0][i].freq) {
				if (fMin == -1) 
					fMin = i; 
				fMax = i;
			}
		if (fMin == -1)
			fMin = fMax = 0;
		freqData.add((uint8_t*)&fMin, 1);			
		freqData.add((uint8_t*)&fMax, 1);
		for (int i = fMin; i <= fMax; i++) {
			uint16_t p = stats[0][i].freq;
			freqData.add((uint8_t*)&p, sizeof(uint16_t));
		}
		auto freqComp = make_shared<GzipCompressionStream<6>>();
		size_t d = dest_offset + sizeof(size_t);
		compressArray(freqComp, freqData, dest, d);
		
		rANSCoder coder;
		coder.initEncode(&dest);
		for (ssize_t i = source_sz - 1; i >= 0; i--) 
			coder.encode(stats[0][source[i]]);
		coder.flush();

		this->compressedCount += dest.size() - dest_offset;
		return dest.size() - dest_offset;
	}

	size_t decompress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) 
	{	
		size_t num = *((size_t*)source);
		source += sizeof(size_t);
		source_sz -= sizeof(size_t);

		Array<uint8_t> freqData(0, MB);
		auto freqComp = make_shared<GzipDecompressionStream>();
		uint8_t *prevSource = source;
		decompressArray(freqComp, source, freqData);
		source_sz -= source - prevSource;
		
		auto stats = vector<vector<rANSCoder::Stat>>(1, vector<rANSCoder::Stat>(AS + 1));
		uint8_t fMin = freqData[0];
		uint8_t fMax = freqData[1];
		for (int i = fMin; i <= fMax; i++) 
			stats[0][i].freq = *(uint16_t*)(freqData.data() + 2 + (i - fMin) * sizeof(uint16_t));
		rANSCoder::Stat::setUpFreqs(stats, 1, AS);

		rANSCoder coder;
		coder.initDecode(source + source_sz - 1);
		dest.resize(dest_offset + num);
		for (size_t i = 0; i < num; i++) {
			uint8_t s = find(stats[0], coder.get());
			*(dest.data() + dest_offset + i) = s;
			coder.decode(stats[0][s]);
			coder.renormalize();
		}
		return num;
	}

	void getCurrentState (Array<uint8_t> &ou) 
	{
	}

	void setCurrentState (uint8_t *in, size_t sz) 
	{
	}
};

#define rANSOrder0DecompressionStream rANSOrder0CompressionStream

#endif // rANSOrder0CompressionStream_H
