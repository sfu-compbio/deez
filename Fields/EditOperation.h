#ifndef EditOperation_H
#define EditOperation_H

#include <string>

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/BzipStream.h"
#include "../Streams/ACGTStream.h"
#include "../Streams/Order0Stream.h"
#include "../Streams/Order2Stream.h"
#include "../Engines/GenericEngine.h"
#include "../Engines/StringEngine.h"
#include "Reference.h"

class SequenceCompressor;
class SequenceDecompressor;

struct EditOperation 
{
	size_t start;
	size_t end;
	std::string seq, MD, op;
	int NM;

	Array<pair<char, int>> ops;

	EditOperation(): NM(-1) {}
	EditOperation(size_t s, const std::string &se, const std::string &op);

	void calculateTags(Reference &reference);
};

class EditOperationCompressor: 
	public GenericCompressor<EditOperation, GzipCompressionStream<6> >
{
	std::vector<CompressionStream*> streams;

	CompressionStream *stitchStream;
	CompressionStream *locationStream;

	const SequenceCompressor &sequence;

public:
	EditOperationCompressor(int blockSize, const SequenceCompressor &seq);
	virtual ~EditOperationCompressor(void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out);
	size_t compressedSize(void);
	void printDetails(void);

private:	
	void addOperation(char op, int seqPos, int size, vector<Array<uint8_t>> &out);
	void addEditOperation(const EditOperation &eo, ACTGStream &nucleotides, vector<Array<uint8_t>> &out);

public:
	friend class FileCompressor;
	enum Fields {
		OPCODES,
		SEQPOS,
		SEQEND,
		XLEN,
		HSLEN,
		LEN,
		OPLEN,
		ACGT,
		// ACGT_N,
		ENUM_COUNT
	};
};

class EditOperationDecompressor: 
	public GenericDecompressor<EditOperation, GzipDecompressionStream>  
{
	std::vector<DecompressionStream*> streams;
	DecompressionStream *stitchStream;
	DecompressionStream *locationStream;

	const SequenceDecompressor &sequence;

public:
	EditOperationDecompressor(int blockSize, const SequenceDecompressor &seq);
	virtual ~EditOperationDecompressor(void);

public:
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *in, size_t in_size);

private:
	EditOperation getEditOperation (size_t loc, ACTGStream &nucleotides, vector<uint8_t*> &fields);
};

#endif // EditOperation_H
