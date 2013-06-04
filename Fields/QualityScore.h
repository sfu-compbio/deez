#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/StringEngine.h"

typedef StringCompressor<GzipCompressionStream<6> >	QualityScoreCompressor;
typedef StringDecompressor<GzipDecompressionStream> QualityScoreDecompressor;

#endif
