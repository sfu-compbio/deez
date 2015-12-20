#ifndef MappingQuality_H
#define MappingQuality_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
#include "../Engines/GenericEngine.h"

class MappingQualityCompressor: 
	public GenericCompressor<uint8_t, GzipCompressionStream<6>> 
{
public:
	MappingQualityCompressor(void):
		GenericCompressor<uint8_t, GzipCompressionStream<6>>()
	{
	}

public:
	void outputRecords (const CircularArray<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k)
	{
		if (!records.size()) { 
			out.resize(0);
			return;
		}
		assert(k <= records.size());

		ZAMAN_START(MappingFlag);
		Array<uint8_t> buffer(k);
		for (size_t i = 0; i < k; i++) {
			uint8_t mf = records[i].getMappingQuality();
			buffer.add(mf);
		}
		compressArray(streams.front(), buffer, out, out_offset);
		ZAMAN_END(MappingFlag);
	}
};

typedef GenericDecompressor<uint8_t, GzipDecompressionStream> 
	MappingQualityDecompressor;

#endif
