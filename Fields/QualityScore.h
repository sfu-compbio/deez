#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/FileGzipStream.h"
#include "../Engines/StringEngine.h"

class QualC: public FileGzipCompressionStream {
	virtual std::string getFilename() { return "qual"; }
};
typedef FileGzipDecompressionStream QualD;

typedef StringCompressor<QualC>	  QualityScoreCompressor;
typedef StringDecompressor<QualD> QualityScoreDecompressor;

#endif
