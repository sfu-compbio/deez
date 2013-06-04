#ifndef ReadName_H
#define ReadName_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"

typedef StringCompressor<GzipCompressionStream<6> >	ReadNameCompressor;
typedef StringDecompressor<GzipDecompressionStream> ReadNameDecompressor;

#endif
