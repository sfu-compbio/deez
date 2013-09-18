#ifndef MappingQuality_H
#define MappingQuality_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
#include "../Engines/GenericEngine.h"

typedef GenericCompressor<uint8_t, AC0CompressionStream> 
	MappingQualityCompressor;
typedef GenericDecompressor<uint8_t, AC0DecompressionStream> 
	MappingQualityDecompressor;

#endif
