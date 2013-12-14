#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order2Stream.h"
#include "../Streams/SAMCompStream.h"
#include "../Streams/MyStream.h"
#include "../Engines/StringEngine.h"

extern char optQuality;
extern char optLossy;

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
	int stat[128];
	int lossy[128];
	bool statMode;

public:
	QualityScoreCompressor (int blockSize);
	virtual ~QualityScoreCompressor (void);

public:
	void addRecord (std::string qual, int flag);
	void addRecord (std::string qual, std::string seq, int flag);
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	
private:
	static double phredScore (char c, int offset);
	void calculateLossyTable (int percentage);
	void lossyTransform (string &qual);
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
