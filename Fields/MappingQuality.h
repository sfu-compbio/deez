#ifndef MappingQuality_H
#define MappingQuality_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/BzipStream.h"
#include "../Engines/GenericEngine.h"

class MappingQualityCompressor: 
	public GenericCompressor<uint8_t, GzipCompressionStream<6, Z_RLE>> 
{
public:
	MappingQualityCompressor(void):
		GenericCompressor<uint8_t, GzipCompressionStream<6, Z_RLE>>()
	{
		if (optBzip) {
			streams[0] = make_shared<BzipCompressionStream>();
		}
	}

public:
	void outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k)
	{
		if (!records.size()) { 
			out.resize(0);
			return;
		}
		assert(k <= records.size());

		ZAMAN_START(MappingQuality);
		Array<uint8_t> buffer(k);
		for (size_t i = 0; i < k; i++) {
			uint8_t mf = records[i].getMappingQuality();
			buffer.add(mf);
		}
		compressArray(streams.front(), buffer, out, out_offset);
		ZAMAN_END(MappingQuality);
	}
};

class MappingQualityDecompressor:
	public GenericDecompressor<uint8_t, GzipDecompressionStream> 
{
public:
	MappingQualityDecompressor (int blockSize): 
		GenericDecompressor<uint8_t, GzipDecompressionStream>(blockSize)
	{
		if (optBzip) {
			streams[0] = make_shared<BzipDecompressionStream>();
		}
	}

public:
	void importRecords (uint8_t *in, size_t in_size) 
	{
		if (in_size == 0) 
			return;
		assert(in_size >= sizeof(size_t));
		
		Array<uint8_t> out;
		size_t s = decompressArray(streams.front(), in, out);
		assert(s % sizeof(uint8_t) == 0);
		records.resize(0);
		records.add((uint8_t*)out.data(), s / sizeof(uint8_t));
	}
};


#endif
