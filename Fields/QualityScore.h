#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/ArithmeticStream.h"
#include "../Streams/ContextModels/Order3Model.h"
#include "../Engines/StringEngine.h"

typedef StringCompressor<ArithmeticCompressionStream<Order3Model> >  	QualityScoreCompressor;
typedef StringDecompressor<ArithmeticDecompressionStream<Order3Model> > QualityScoreDecompressor;

#endif
