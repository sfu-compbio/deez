#ifndef MappingFlag_H
#define MappingFlag_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/BzipStream.h"
#include "../Engines/GenericEngine.h"

class MappingFlagCompressor: 
	public GenericCompressor<uint16_t, GzipCompressionStream<6>> 
{
public:
	MappingFlagCompressor(void):
		GenericCompressor<uint16_t, GzipCompressionStream<6>>()
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

		ZAMAN_START(MappingFlag);
		Array<uint8_t> buffer(k * sizeof(uint16_t));
		for (size_t i = 0; i < k; i++) {
			uint16_t mf = records[i].getMappingFlag();
			buffer.add((uint8_t*)&mf, sizeof(uint16_t));
		}
		compressArray(streams.front(), buffer, out, out_offset);
		ZAMAN_END(MappingFlag);
	}
};

class MappingFlagDecompressor:
	public GenericDecompressor<uint16_t, GzipDecompressionStream> 
{
public:
	MappingFlagDecompressor (int blockSize): 
		GenericDecompressor<uint16_t, GzipDecompressionStream>(blockSize)
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
		assert(s % sizeof(uint16_t) == 0);
		records.resize(0);
		records.add((uint16_t*)out.data(), s / sizeof(uint16_t));
	}
};

#endif
