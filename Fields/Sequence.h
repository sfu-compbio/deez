#ifndef ReferenceFixes_H
#define ReferenceFixes_H

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "../Common.h"
#include "../Stats.h"
#include "../Reference.h"
#include "../Engines/Engine.h"
#include "../Engines/GenericEngine.h"
#include "../Engines/StringEngine.h"
#include "../Fields/EditOperation.h"
#include "../Streams/GzipStream.h"
#include "EditOperation.h"

class SequenceCompressor: public Compressor {
	friend class Stats;
	
	Reference reference;
	
	CompressionStream *fixesStream;
	CompressionStream *fixesReplaceStream;

	Array<uint8_t> fixes_loc;
	Array<uint8_t> fixes_replace;

	std::string chromosome; // chromosome index

	char *fixed;
	size_t fixedStart, fixedEnd;
	size_t maxEnd;

public:
	SequenceCompressor (const std::string &refFile, int bs);
	~SequenceCompressor (void);

public:
	void updateBoundary (size_t loc);
	void outputRecords (Array<uint8_t> &output, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out) { out.resize(0); }
	size_t compressedSize(void) { 
		LOGN("[Fixes %lu Replace %lu]", fixesStream->getCount(), fixesReplaceStream->getCount());
		return fixesStream->getCount() + fixesReplaceStream->getCount(); 
	}

	size_t applyFixes (size_t end, EditOperationCompressor &editOperation, size_t&, size_t&, size_t&, size_t&, size_t&);
	
public:
	std::string getChromosome (void) const { return chromosome; }
	void scanChromosome (const std::string &s);
	size_t getChromosomeLength (void) const { return reference.getChromosomeLength(chromosome); }
	void scanSAMComment (const string &comment) { reference.scanSAMComment(comment); } 

private:
	static void applyFixesThread(EditOperationCompressor &editOperation, Array<int*> &stats, size_t fixedStart, size_t offset, size_t size);
	static void updateGenomeLoc (size_t loc, char ch, Array<int*> &stats);
};

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
	void scanSAMComment (const string &comment) { reference.scanSAMComment(comment); } 
};

#endif
