#ifndef MappingQuality_H
#define MappingQuality_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
#include "../Engines/GenericEngine.h"

typedef GenericCompressor<uint8_t, GzipCompressionStream<6>> 
	MappingQualityCompressor;
typedef GenericDecompressor<uint8_t, GzipDecompressionStream> 
	MappingQualityDecompressor;

#endif
