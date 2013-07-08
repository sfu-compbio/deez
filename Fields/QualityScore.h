#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/ArithmeticStream.h"
#include "../Streams/GzipStream.h"
#include "../Streams/ContextModels/Order3Model.h"
#include "../Engines/StringEngine.h"

//typedef StringCompressor<GzipCompressionStream<6> > QualityScoreCompressor;
//typedef StringDecompressor<GzipDecompressionStream> QualityScoreDecompressor;

class QualityScoreCompressor: public StringCompressor<ArithmeticCompressionStream<Order3Model> > {
	virtual const char *getID () const { return "QualityScore"; }

public:
	QualityScoreCompressor(int blockSize):
		StringCompressor<ArithmeticCompressionStream<Order3Model> >(blockSize) {}
};

class QualityScoreDecompressor: public StringDecompressor<ArithmeticDecompressionStream<Order3Model> > {
	virtual const char *getID () const { return "QualityScore"; }

public:
	QualityScoreDecompressor(int blockSize):
		StringDecompressor<ArithmeticDecompressionStream<Order3Model> >(blockSize) {}
};

#endif
