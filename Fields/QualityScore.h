#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order2Stream.h"
#include "../Engines/StringEngine.h"

class QualityScoreCompressor: 
	public StringCompressor<AC2CompressionStream<64> > 
{	
public:
	QualityScoreCompressor (int blockSize);
	virtual ~QualityScoreCompressor (void);

public:
	void addRecord (std::string qual, int flag);
	void addRecord (std::string qual, std::string seq, int flag);
};

class QualityScoreDecompressor: 
	public StringDecompressor<AC2DecompressionStream<64> > 
{
public:
	QualityScoreDecompressor (int blockSize);
	virtual ~QualityScoreDecompressor (void);

public:
	std::string getRecord (size_t seq_len, int flag);
};

#endif
