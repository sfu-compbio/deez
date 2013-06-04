#ifndef EditOperation_H
#define EditOperation_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"

typedef StringCompressor<GzipCompressionStream<6> >	EditOperationCompressor;
typedef StringDecompressor<GzipDecompressionStream> EditOperationDecompressor;

#endif
