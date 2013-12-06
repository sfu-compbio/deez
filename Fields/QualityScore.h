#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order2Stream.h"
#include "../Streams/BStream.h"
#include "../Streams/MyStream.h"
#include "../Engines/StringEngine.h"

typedef 
	//GzipCompressionStream<6>
	AC2CompressionStream<64>
	QualityCompressionStream;
typedef 
	//GzipDecompressionStream
	AC2DecompressionStream<64>
	QualityDecompressionStream;

class QualityScoreCompressor: 
	public StringCompressor<QualityCompressionStream> 
{	
public:
	QualityScoreCompressor (int blockSize);
	virtual ~QualityScoreCompressor (void);

public:
	void addRecord (std::string qual, int flag);
	void addRecord (std::string qual, std::string seq, int flag);
};

class QualityScoreDecompressor: 
	public StringDecompressor<QualityDecompressionStream> 
{
public:
	QualityScoreDecompressor (int blockSize);
	virtual ~QualityScoreDecompressor (void);

public:
	std::string getRecord (size_t seq_len, int flag);
};

#endif
