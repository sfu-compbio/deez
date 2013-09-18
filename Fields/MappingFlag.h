#ifndef MappingFlag_H
#define MappingFlag_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
#include "../Engines/GenericEngine.h"

typedef GenericCompressor<uint16_t, AC0CompressionStream>   
	MappingFlagCompressor;
typedef GenericDecompressor<uint16_t, AC0DecompressionStream> 
	MappingFlagDecompressor;

#endif
