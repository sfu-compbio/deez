#pragma once

#include <map>
#include <string>

#include "Common.h"
#include "Engines.h"
#include "Streams.h"
#include "FileIO.h"

namespace Legacy {
namespace v11 {

const int QualRange = 96;

/****************************************************************************************/

struct EditOperation {
	size_t start;
	size_t end;
	std::string seq;
	std::string op;
	
	EditOperation() {}
	EditOperation(size_t s, const std::string &se, const std::string &o) :
		start(s), seq(se), op(o)
	{
		end = s;
		size_t size = 0;
		if (op != "*") for (size_t pos = 0; pos < op.length(); pos++) {
			if (isdigit(op[pos])) {
				size = size * 10 + (op[pos] - '0');
				continue;
			}
			switch (op[pos]) {
				case 'M':
				case '=':
				case 'X':
				case 'D':
				case 'N':
					end += size;
					break;
				default:
					break;
			}
			size = 0;
		}
	}
};

struct PairedEndInfo {
	std::string chr;
	size_t pos;
	int32_t tlen;
	
	PairedEndInfo (void) {}
	PairedEndInfo (const std::string &c, size_t p, int32_t t, const std::string &mc, size_t mpos):
		chr(c), pos(p), tlen(t) 
	{
		if ( (mc == "=") && !((mpos <= pos && tlen <= 0) || (mpos > pos && tlen >= 0)) )
			throw DZException("Mate position and template length inconsistent! pos=%lu matepos=%lu tlen=%d", mpos, pos, tlen);
		if (mc == "=") {
			if (tlen <= 0)
				pos -= mpos;
			else
				pos = mpos - pos;
		}
		pos++;
	}
};

/****************************************************************************************/

class EditOperationDecompressor;
class SequenceDecompressor: public Decompressor {
	Reference reference;
	
	DecompressionStream *fixesStream;
	DecompressionStream *fixesReplaceStream;

	char   *fixed;
	size_t fixedStart, fixedEnd;
	
	std::string chromosome;

public:
	SequenceDecompressor (const std::string &refFile, int bs);
	~SequenceDecompressor (void);

public:
	bool hasRecord (void);
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *, size_t) {}

public:
	void setFixed (EditOperationDecompressor &editOperation);
	void scanChromosome (const std::string &s);
	std::string getChromosome (void) const { return chromosome; }
	void scanSAMComment (const std::string &comment) { reference.scanSAMComment(comment); } 
};

class EditOperationDecompressor: 
	public GenericDecompressor<EditOperation, GzipDecompressionStream>  
{
	DecompressionStream *unknownStream;
	DecompressionStream *operandStream;
	DecompressionStream *lengthStream;

	DecompressionStream *stitchStream;
	DecompressionStream *locationStream;

	char *fixed;
	size_t fixedStart;

public:
	EditOperationDecompressor(int blockSize);
	virtual ~EditOperationDecompressor(void);

public:
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *in, size_t in_size);

private:
	EditOperation getEditOperation (size_t loc, ACTGStream &nucleotides, uint8_t *&op, uint8_t *&len);

	friend class SequenceDecompressor;
	void setFixed(char *f, size_t fs);
};

typedef GenericDecompressor<uint16_t, GzipDecompressionStream> 
	MappingFlagDecompressor;

typedef GenericDecompressor<uint8_t, GzipDecompressionStream> 
	MappingQualityDecompressor;

typedef StringDecompressor<GzipDecompressionStream> 
	OptionalFieldDecompressor;

class PairedEndDecompressor: 
	public GenericDecompressor<PairedEndInfo, GzipDecompressionStream> 
{
public:
	PairedEndDecompressor (int blockSize);
	virtual ~PairedEndDecompressor (void);
	
public:
	void importRecords (uint8_t *in, size_t in_size);
	const PairedEndInfo &getRecord (const std::string &mc, size_t mpos);
};

typedef 
	AC2DecompressionStream<QualRange>
	QualityDecompressionStream;

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

class ReadNameDecompressor: 
	public StringDecompressor<GzipDecompressionStream>  
{
	DecompressionStream *indexStream;

public:
	ReadNameDecompressor(int blockSize);
	virtual ~ReadNameDecompressor(void);

public:
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *in, size_t in_size);
};

};
};
