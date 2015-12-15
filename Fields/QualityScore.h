#ifndef QualityScore_H
#define QualityScore_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order2Stream.h"
#include "../Streams/SAMCompStream.h"
#include "../Engines/StringEngine.h"

extern char optQuality;
extern char optLossy;
extern bool optNoQual;

const int QualRange = 96;

typedef 
	AC2CompressionStream<AC, QualRange>
	QualityCompressionStream;
typedef 
	AC2DecompressionStream<AC, QualRange>
	QualityDecompressionStream;

class QualityScoreCompressor: 
	public StringCompressor<QualityCompressionStream> 
{	
	char offset;
	int stat[128];
	int lossy[128];
	bool statMode;

public:
	QualityScoreCompressor (int blockSize);
	virtual ~QualityScoreCompressor (void);

public:
	void addRecord (const std::string &qual, int flag);
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	std::string shrink(const std::string &s, int flag);
	
private:
	static double phredScore (char c, int offset);
	char calculateOffset (void);
	void calculateLossyTable (int percentage);
	void lossyTransform (string &qual);
};

class QualityScoreDecompressor: 
	public StringDecompressor<QualityDecompressionStream> 
{
	char offset;
	char sought;

public:
	QualityScoreDecompressor (int blockSize);
	virtual ~QualityScoreDecompressor (void);

public:
	std::string getRecord (size_t seq_len, int flag);
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *in, size_t in_size);
};

#endif
