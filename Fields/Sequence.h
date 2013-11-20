#ifndef ReferenceFixes_H
#define ReferenceFixes_H

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "../Common.h"
#include "../Reference.h"
#include "../Engines/Engine.h"
#include "../Engines/GenericEngine.h"
#include "../Engines/StringEngine.h"
#include "../Streams/GzipStream.h"

struct EditOP {
	size_t start;
	size_t end;
	std::string seq;
	std::string op;

	EditOP() {}
	EditOP(size_t s, const std::string &se, const std::string &o) :
		start(s), seq(se), op(o) {}
};

typedef StringCompressor<GzipCompressionStream<6> > 
	EditOperationCompressor;
typedef StringDecompressor<GzipDecompressionStream> 
	EditOperationDecompressor;

class SequenceCompressor: public Compressor {
	Reference reference;
	EditOperationCompressor editOperation;
	
	CompressionStream *fixesStream;
	CompressionStream *fixesReplaceStream;

	// temporary for the Cigars before the genome fixing
	CircularArray<EditOP> records;

	Array<uint32_t> fixes_loc;
	Array<uint8_t>  fixes_replace;

	std::string chromosome; // chromosome index
	

	char *fixed;
	size_t fixedStart, fixedEnd;
	size_t maxEnd;

public:
	SequenceCompressor (const std::string &refFile, int bs);
	~SequenceCompressor (void);

public:
	void addRecord (size_t loc, const std::string &seq, const std::string &op);
	void outputRecords (Array<uint8_t> &output, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out) { out.resize(0); }

	size_t applyFixes (size_t end, size_t&, size_t&, size_t&, size_t&, size_t&);
	
public:
	std::string getChromosome (void) const { return chromosome; }
	void scanChromosome (const std::string &s);
	size_t size(void) { return records.size(); }
	
private:
	void updateGenomeLoc (size_t loc, char ch, Array<int*> &stats);
	std::string getEditOP (size_t loc, const std::string &seq, const std::string &op);
};

class SequenceDecompressor: public Decompressor {
	Reference reference;
	EditOperationDecompressor editOperation;
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
	EditOP getRecord (size_t loc);

	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *, size_t) {}

public:
	void scanChromosome (const std::string &s);
	std::string getChromosome (void) const { return chromosome; }
	size_t importFixes (uint8_t *in, size_t in_size);

private:
	EditOP getSeqCigar (size_t loc, const std::string &op);	
};

#endif
