#ifndef MappingFlag_H
#define MappingFlag_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

typedef GenericCompressor<short, GzipCompressionStream<6> >	MappingFlagCompressor;
typedef GenericDecompressor<short, GzipDecompressionStream>	MappingFlagDecompressor;

#endif
