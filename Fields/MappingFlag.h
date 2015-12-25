#ifndef MappingFlag_H
#define MappingFlag_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

class MappingFlagCompressor: 
	public GenericCompressor<uint16_t, GzipCompressionStream<6>> 
{
public:
	MappingFlagCompressor(void):
		GenericCompressor<uint16_t, GzipCompressionStream<6>>()
	{
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

typedef GenericDecompressor<uint16_t, GzipDecompressionStream> 
	MappingFlagDecompressor;

#endif
