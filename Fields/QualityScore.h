#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/FileGzipStream.h"
#include "../Engines/StringEngine.h"

typedef StringCompressor<FileGzipCompressionStream>	  QualityScoreCompressor;
typedef StringDecompressor<FileGzipDecompressionStream> QualityScoreDecompressor;

#endif
