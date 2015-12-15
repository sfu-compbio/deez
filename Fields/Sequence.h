#ifndef ReferenceFixes_H
#define ReferenceFixes_H

#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <smmintrin.h>

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
	CompressionStream
		*fixesStream,
		*fixesReplaceStream;
	
	Array<uint8_t> 
		fixesLoc,
		fixesReplace;
	
	std::string chromosome; // chromosome index
	std::string fixed;
	size_t fixedStart, fixedEnd;
	size_t maxEnd;

public:
	SequenceCompressor (const std::string &refFile, int bs);
	~SequenceCompressor (void);

public:
	void updateBoundary (size_t loc);
	size_t getBoundary() const { return maxEnd; };
	void outputRecords (Array<uint8_t> &output, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out) { out.resize(0); }
	size_t compressedSize(void);
	void printDetails(void);

	size_t applyFixes (size_t end, EditOperationCompressor &editOperation, size_t&, size_t&, size_t&, size_t&, size_t&);
	
public:
	std::string getChromosome (void) const { return chromosome; }
	void scanChromosome (const std::string &s);
	size_t getChromosomeLength (void) const { return reference.getChromosomeLength(chromosome); }
	void scanSAMComment (const string &comment) { reference.scanSAMComment(comment); } 
	char operator[] (size_t pos) const;
	Reference &getReference() { return reference; }

private:
	#ifdef __SSE2__
		typedef std::vector<__m128i> Stats;
	#else
		typedef std::vector<std::array<uint16_t, 6>> Stats;
	#endif
	static void applyFixesThread(EditOperationCompressor &editOperation, Stats &stats, 
		size_t fixedStart, size_t offset, size_t size);
	static void updateGenomeLoc (size_t loc, char ch, Stats &stats);
};

class SequenceDecompressor: public Decompressor {
	Reference reference;
	DecompressionStream *fixesStream;
	DecompressionStream *fixesReplaceStream;

private:
	std::string chromosome;
	std::string fixed;
	size_t fixedStart, fixedEnd;

public:
	SequenceDecompressor (const std::string &refFile, int bs);
	~SequenceDecompressor (void);

public:
	bool hasRecord (void);
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *, size_t) {}

public:
	void scanChromosome (const std::string &s);
	std::string getChromosome (void) const { return chromosome; }
	void scanSAMComment (const string &comment) { reference.scanSAMComment(comment); } 
	char operator[] (size_t pos) const;
	const Reference &getReference() const { return reference; }
};

#endif
