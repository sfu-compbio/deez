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
#include <cpuid.h>

#include "../Common.h"
#include "../Stats.h"
#include "../Reference.h"
#include "../Engines/Engine.h"
#include "../Engines/GenericEngine.h"
#include "../Engines/StringEngine.h"
#include "../Fields/EditOperation.h"
#include "../Streams/GzipStream.h"
#include "../Streams/BzipStream.h"
#include "../Streams/ACGTStream.h"
#include "EditOperation.h"
#include "SAMComment.h"

class GenomeStats {
protected:
	std::vector<array<uint16_t, 6>> stats;

public:
	GenomeStats () {}
	GenomeStats (size_t size): stats(size, array<uint16_t, 6>()) {}

	virtual void updateGenomeLoc (size_t loc, char ch) {
		assert(loc < stats.size());
		stats[loc][getDNAValue(ch)]++;
	}

	virtual void update (size_t g, shared_ptr<GenomeStats> threadStats, size_t i) {
		for (int j = 0; j < 6; j++)
			stats[g][j] += threadStats->stats[i][j];
	}

	virtual int maxPos (size_t i) {
		int max = -1, pos = -1;
		for (int j = 1; j < 6; j++) {
			if (stats[i][j] > max)
				max = stats[i][pos = j];
		}
		return pos;
	}

	virtual size_t size () {
		return stats.size();
	}
};

inline void cpuid(int info[4], int InfoType)
{
	__cpuid_count(InfoType, 0, info[0], info[1], info[2], info[3]);
}

// Based on http://stackoverflow.com/questions/6121792/how-to-check-if-a-cpu-supports-the-sse3-instruction-set
inline bool supports_sse41()
{
	int info[4];
	cpuid(info, 0);
	int nIds = info[0];
	if (nIds >= 0x00000001){
		return (info[2] & ((int)1 << 19)) != 0;
	}
	return false;
}

class GenomeStatsSSE: public GenomeStats {
	std::vector<__m128i> statsSSE;
	
	static const __m128i getSSEvalueCache[]; 
	static const __m128i invert;

public:
	static shared_ptr<GenomeStats> newStats (size_t sz) {
		if (supports_sse41()) {
			return make_shared<GenomeStatsSSE>(sz);
		} else {
			return make_shared<GenomeStats>(sz);
		}
	}

public:
	GenomeStatsSSE (size_t size): statsSSE(size, _mm_setzero_si128()) {}

	void updateGenomeLoc (size_t loc, char ch) {
		assert(loc < statsSSE.size());
		statsSSE[loc] = _mm_add_epi16(statsSSE[loc], getSSEvalueCache[getDNAValue(ch)]);
	}

	void update (size_t g, shared_ptr<GenomeStats> threadStats, size_t i) {
		statsSSE[g] = _mm_add_epi16(statsSSE[g], static_cast<GenomeStatsSSE*>(threadStats.get())->statsSSE[i]);
	}

	int maxPos (size_t i) {
		if (_mm_movemask_epi8(_mm_cmpeq_epi8(statsSSE[i], _mm_setzero_si128())) == 0xFFFF)
			return -1;
		return _mm_extract_epi16(_mm_minpos_epu16(_mm_sub_epi16(invert, statsSSE[i])), 1);
	}

	size_t size () {
		return statsSSE.size();
	}
};

class SequenceCompressor: public Compressor {	
	friend class Stats;
	Reference reference;
	
	Array<uint8_t> 
		fixesLoc,
		fixesReplace;
	Array<uint8_t>
		fixesLocSt;
	
	std::string chromosome; // chromosome index
	std::string fixed;
	size_t fixedStart, fixedEnd;
	size_t maxEnd;

public:
	SequenceCompressor (const std::string &refFile);

public:
	void updateBoundary (size_t loc);
	size_t getBoundary() const { return maxEnd; };
	void outputRecords (const Array<Record> &records, Array<uint8_t> &output, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out) { out.resize(0); }
	void printDetails(void);

	size_t applyFixes (size_t end, const Array<Record> &records, const Array<EditOperation> &editOps, size_t&, size_t&, size_t&, size_t&, size_t&);

	size_t currentMemoryUsage() {
		LOG("Reference uses %'lu", reference.currentMemoryUsage());
		return 
			sizeInMemory(fixesLoc) + sizeInMemory(fixesReplace) +
			sizeInMemory(chromosome) + sizeInMemory(fixed) + sizeInMemory(streams) +
			sizeof(size_t) * 3 + reference.currentMemoryUsage();
	}

	
public:
	std::string getChromosome (void) const { return chromosome; }
	void scanChromosome (const std::string &s, const SAMComment &samComment);
	size_t getChromosomeLength (void) const { return reference.getChromosomeLength(chromosome); }
	char operator[] (size_t pos) const;
	Reference &getReference() { return reference; }

private:
	static void applyFixesThread(const Array<Record> &records, const Array<EditOperation> &editOps, shared_ptr<GenomeStats> &stats, 
		size_t fixedStart, size_t offset, size_t size);

public:
	enum Fields {
		FIXES,
		FIXES_ST,
		REPLACE,
		ENUM_COUNT
	};
};

class SequenceDecompressor: public Decompressor {
	Reference reference;

private:
	std::string chromosome;
	std::string fixed;
	size_t fixedStart, fixedEnd;

public:
	SequenceDecompressor (const std::string &refFile, int bs);

public:
	bool hasRecord (void);
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *, size_t) {}

public:
	void scanChromosome (const std::string &s, const SAMComment &samComment);
	std::string getChromosome (void) const { return chromosome; }
	char operator[] (size_t pos) const;
	const Reference &getReference() const { return reference; }
};

#endif
