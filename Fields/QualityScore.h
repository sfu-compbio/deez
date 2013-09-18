#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order2Stream.h"
#include "../Engines/StringEngine.h"

class QualityScoreCompressor: 
	public StringCompressor<AC2CompressionStream> {
	
public:
	QualityScoreCompressor (int blockSize);
	virtual ~QualityScoreCompressor (void);

public:
	void addRecord (std::string qual, int flag);
};

class QualityScoreDecompressor: 
	public StringDecompressor<AC2DecompressionStream> {

public:
	QualityScoreDecompressor (int blockSize);
	virtual ~QualityScoreDecompressor (void);

public:
	std::string getRecord (size_t seq_len, int flag);
};

#endif
