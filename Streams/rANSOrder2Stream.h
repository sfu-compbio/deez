#ifndef rANSOrder2CompressionStream_H
#define rANSOrder2CompressionStream_H

#include <vector>
#include "../Common.h"
#include "rANSCoder.h"	

template<int AS>
class rANSOrder2CompressionStream: public CompressionStream, public DecompressionStream {
	vector<vector<rANSCoder::Stat>> stats;
	vector<vector<uint8_t>> lookup;
public:
	rANSOrder2CompressionStream()
	{
	}

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

#define RANS1
#ifdef RANS1
	size_t compress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) 
	{
		dest.resize(dest_offset + sizeof(size_t));
		memcpy(dest.data() + dest_offset, &source_sz, sizeof(size_t));

		if (!stats.size()) 
			stats.resize(AS, vector<rANSCoder::Stat>(AS + 1));

		uint32_t context = 0, q1 = 0;
		for (ssize_t i = 0; i < source_sz; i++) { 
			this->stats[context][source[i]].freq++;
			context = source[i];
		}
		rANSCoder::Stat::setUpFreqs(stats, AS, AS);

		Array<uint8_t> freqData(0, MB);
		for (uint16_t c = 0; c < AS; c++) {
			char fMin = -1, fMax;
			for (int i = 0; i < AS; i++)
				if (stats[c][i].freq) {
					if (fMin == -1) 
						fMin = i; 
					fMax = i;
				}
			if (fMin == -1)
				continue;
			freqData.add((uint8_t*)&c, sizeof(uint16_t));
			freqData.add((uint8_t*)&fMin, 1);			
			freqData.add((uint8_t*)&fMax, 1);
			for (int i = fMin; i <= fMax; i++) {
				uint16_t p = stats[c][i].freq;
				freqData.add((uint8_t*)&p, sizeof(uint16_t));
			}
		}
		auto freqComp = make_shared<GzipCompressionStream<6>>();
		size_t d = dest_offset + sizeof(size_t);
		compressArray(freqComp, freqData, dest, d);

		rANSCoder coder;
		coder.initEncode(&dest);
		for (ssize_t i = source_sz - 1; i > 0; i--) 
			coder.encode(stats[source[i - 1]][source[i]]);
		coder.encode(stats[0][source[0]]);
		coder.flush();
		
		this->compressedCount += dest.size() - dest_offset;
		return dest.size() - dest_offset;
	}

	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{	
		size_t num = *((size_t*)source);
		source += sizeof(size_t);
		source_sz -= sizeof(size_t);

		Array<uint8_t> freqData(0, MB);
		auto freqComp = make_shared<GzipDecompressionStream>();
		uint8_t *prevSource = source;
		decompressArray(freqComp, source, freqData);
		source_sz -= source - prevSource;

		if (!stats.size()) 
			stats.resize(AS, vector<rANSCoder::Stat>(AS + 1));
		for (size_t i = 0; i < freqData.size(); ) {
			uint16_t c = *(uint16_t*)(freqData.data() + i); i += sizeof(uint16_t);
			uint8_t fMin = freqData[i++];
			uint8_t fMax = freqData[i++];
			for (int j = fMin; j <= fMax; j++) 
				stats[c][j].freq = *(uint16_t*)(freqData.data() + i), i += sizeof(uint16_t);
		}
		rANSCoder::Stat::setUpFreqs(stats, AS, AS);
		
		if (!lookup.size()) 
			lookup.resize(AS, vector<uint8_t>(rANSCoder::SCALE));
		for (int i = 0; i < stats.size(); i++) {
			for (int j = 0; j < stats[i].size(); j++) {
				for (int k = stats[i][j].start; k < stats[i][j].start + stats[i][j].freq; k++) {
					lookup[i][k] = j;
				}
			}
		}

		rANSCoder coder;
		coder.initDecode(source + source_sz - 1);
		dest.resize(dest_offset + num);
		uint16_t context = 0, q1 = 0;
		for (size_t i = 0; i < num; i++) {
			uint8_t s = lookup[context][coder.get()];
			// uint8_t s = find(stats[context], coder.get());
			*(dest.data() + dest_offset + i) = s;
			coder.decode(stats[context][s]);
			coder.renormalize();
			context = s;
		}
		return num;
	}

#else
	size_t compress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) 
	{
		dest.resize(dest_offset + sizeof(size_t));
		memcpy(dest.data() + dest_offset, &source_sz, sizeof(size_t));

		if (!stats.size()) 
			stats.resize(AS * AS, vector<rANSCoder::Stat>(AS + 1));

		uint32_t context = 0, q1 = 0;
		for (ssize_t i = 0; i < source_sz; i++) { 
			this->stats[context][source[i]].freq++;
			context = q1 * AS + source[i], q1 = source[i];
		}
		rANSCoder::Stat::setUpFreqs(stats, AS * AS, AS);

		Array<uint8_t> freqData(0, MB);
		for (uint16_t c = 0; c < AS * AS; c++) {
			char fMin = -1, fMax;
			for (int i = 0; i < AS; i++)
				if (stats[c][i].freq) {
					if (fMin == -1) 
						fMin = i; 
					fMax = i;
				}
			if (fMin == -1)
				continue;
			freqData.add((uint8_t*)&c, sizeof(uint16_t));
			freqData.add((uint8_t*)&fMin, 1);			
			freqData.add((uint8_t*)&fMax, 1);
			for (int i = fMin; i <= fMax; i++) {
				uint16_t p = stats[c][i].freq;
				freqData.add((uint8_t*)&p, sizeof(uint16_t));
			}
		}
		auto freqComp = make_shared<GzipCompressionStream<6>>();
		size_t d = dest_offset + sizeof(size_t);
		compressArray(freqComp, freqData, dest, d);

		rANSCoder coder;
		coder.initEncode(&dest);
		for (ssize_t i = source_sz - 1; i > 1; i--) 
			coder.encode(stats[source[i - 2] * AS + source[i - 1]][source[i]]);
		coder.encode(stats[source[0]][source[1]]);
		coder.encode(stats[0][source[0]]);
		coder.flush();
		
		// rANSCoder coder[2];
		// coder[0].initEncode(&dest);
		// coder[1].initEncode(&dest);
		// size_t last = source_sz - 1;
		// if (source_sz % 2) {
		// 	coder[0].encode(stats[source[source_sz - 3] * AS + source[source_sz - 2]][source[source_sz - 1]]);
		// 	last--;
		// }
		// // 6... 5,4    3,2    1,0
		// // 7.. 
		// for (ssize_t i = last; i > 1; i--) {
		// 	coder[0].encode(stats[source[i - 2] * AS + source[i - 1]][source[i]]);
		// 	i--;
		// 	coder[1].encode(stats[source[i - 2] * AS + source[i - 1]][source[i]]);
		// }
		// coder[0].encode(stats[source[0]][source[1]]);
		// coder[1].encode(stats[0][source[0]]);
		// coder[0].flush();
		// coder[1].flush();

		this->compressedCount += dest.size() - dest_offset;
		return dest.size() - dest_offset;
	}

	size_t decompress (uint8_t *source, size_t source_sz, 
			Array<uint8_t> &dest, size_t dest_offset) 
	{	
		size_t num = *((size_t*)source);
		source += sizeof(size_t);
		source_sz -= sizeof(size_t);

		Array<uint8_t> freqData(0, MB);
		auto freqComp = make_shared<GzipDecompressionStream>();
		uint8_t *prevSource = source;
		decompressArray(freqComp, source, freqData);
		source_sz -= source - prevSource;

		if (!stats.size()) 
			stats.resize(AS * AS, vector<rANSCoder::Stat>(AS + 1));

		for (size_t i = 0; i < freqData.size(); ) {
			uint16_t c = *(uint16_t*)(freqData.data() + i); i += sizeof(uint16_t);
			uint8_t fMin = freqData[i++];
			uint8_t fMax = freqData[i++];
			for (int j = fMin; j <= fMax; j++) 
				stats[c][j].freq = *(uint16_t*)(freqData.data() + i), i += sizeof(uint16_t);
		}
		rANSCoder::Stat::setUpFreqs(stats, AS * AS, AS);
		
		// if (!lookup.size()) 
		// 	lookup.resize(AS * AS, vector<uint8_t>(rANSCoder::SCALE));
		// for (int i = 0; i < stats.size(); i++) {
		// 	for (int j = 0; j < stats[i].size(); j++) {
		// 		for (int k = stats[i][j].start; k < stats[i][j].start + stats[i][j].freq; k++) 
		// 			lookup[i][k] = j;
		// 	}
		// }

		rANSCoder coder;
		coder.initDecode(source + source_sz - 1);
		dest.resize(dest_offset + num);
		uint16_t context = 0, q1 = 0;
		for (size_t i = 0; i < num; i++) {
			//uint8_t s = lookup[context][coder.get()];
			uint8_t s = find(stats[context], coder.get());
			*(dest.data() + dest_offset + i) = s;
			coder.decode(stats[context][s]);
			coder.renormalize();
			context = q1 * AS + s, q1 = s;
		}
		return num;
	}
#endif

	void getCurrentState (Array<uint8_t> &ou) 
	{
	}

	void setCurrentState (uint8_t *in, size_t sz) 
	{
	}
};

#define rANSOrder2DecompressionStream rANSOrder2CompressionStream

#endif // rANSOrder2CompressionStream_H
