#ifndef EditOperation_H
#define EditOperation_H

#include <string>

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
#include "../Engines/GenericEngine.h"
#include "../Engines/StringEngine.h"

class SequenceCompressor;
class SequenceDecompressor;

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

class EditOperationCompressor: 
	public GenericCompressor<EditOperation, GzipCompressionStream<6> >
{
	CompressionStream *unknownStream;
	CompressionStream *operandStream;
	CompressionStream *lengthStream;
	CompressionStream *stitchStream;
	CompressionStream *locationStream;

	char *fixed;
	size_t fixedStart;

private:
	uint8_t sequence;
	uint8_t Nfixes;
	int sequencePos, NfixesPos;

public:
	EditOperationCompressor(int blockSize);
	virtual ~EditOperationCompressor(void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out);
	size_t compressedSize(void) { 
		LOGN("[NUC %lu UNK %lu OP %lu LEN %lu LOC %lu STI %lu]", 
			stream->getCount(),
			unknownStream->getCount(),
			operandStream->getCount(),
			lengthStream->getCount(),
			locationStream->getCount(),
			stitchStream->getCount()
		);
		return stream->getCount() + unknownStream->getCount() +
			operandStream->getCount() + lengthStream->getCount() +
			locationStream->getCount() + stitchStream->getCount();
	}
	
private:
	friend class SequenceCompressor;
	void setFixed(char *f, size_t fs);
	const EditOperation &operator[] (int idx);
	
	void addOperation(char op, int size,
		Array<uint8_t> &operands, Array<uint8_t> &lengths);
	void addSequence (const char *seq, size_t len, 
		Array<uint8_t> &nucleotides, Array<uint8_t> &unknowns);
	void addEditOperation(const EditOperation &eo,
		Array<uint8_t> &nucleotides, Array<uint8_t> &unknowns, 
		Array<uint8_t> &operands, Array<uint8_t> &lengths);

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

private:
	const uint8_t *sequence;
	const uint8_t *Nfixes;
	int sequencePos, NfixesPos;

public:
	EditOperationDecompressor(int blockSize);
	virtual ~EditOperationDecompressor(void);

public:
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *in, size_t in_size);

private:
	void getSequence(std::string &out, size_t sz);
	EditOperation getEditOperation (size_t loc, uint8_t *&op, uint8_t *&len);

	friend class SequenceDecompressor;
	void setFixed(char *f, size_t fs);
};

#endif // EditOperation_H
