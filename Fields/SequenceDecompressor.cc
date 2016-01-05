#include <thread>
#include <utility>
#include "Sequence.h"
#include "../Streams/rANSOrder0Stream.h"
using namespace std;

SequenceDecompressor::SequenceDecompressor (const string &refFile, int bs):
	reference(refFile), 
	chromosome("")
{
	streams.resize(SequenceCompressor::Fields::ENUM_COUNT);
	if (optBzip) {
		for (int i = 0; i < streams.size(); i++)
			streams[i] = make_shared<BzipDecompressionStream>();	
	} else {
		for (int i = 0; i < streams.size(); i++)
			streams[i] = make_shared<GzipDecompressionStream>();
	}
	streams[SequenceCompressor::Fields::FIXES] = make_shared<rANSOrder0DecompressionStream<256>>();
}

bool SequenceDecompressor::hasRecord (void) 
{
	return true;
}

void SequenceDecompressor::importRecords (uint8_t *in, size_t in_size)  
{
	if (chromosome == "*" || in_size == 0)
		return;

	ZAMAN_START(Sequence);

	size_t newFixedStart = *(size_t*)in; in += sizeof(size_t);
	size_t newFixedEnd   = *(size_t*)in; in += sizeof(size_t);
	
	Array<uint8_t> fixes_loc;
	decompressArray(streams[SequenceCompressor::Fields::FIXES], in, fixes_loc);
	Array<uint8_t> fixes_st;
	decompressArray(streams[SequenceCompressor::Fields::FIXES_ST], in, fixes_st);
	Array<uint8_t> fixes_replace;
	decompressArray(streams[SequenceCompressor::Fields::REPLACE], in, fixes_replace);

	ZAMAN_START(Load);

	assert(fixedStart <= newFixedStart);	
	if (newFixedStart < fixedEnd) { // Copy old fixes
		fixed = fixed.substr(newFixedStart - fixedStart, fixedEnd - newFixedStart);
		fixed += reference.copy(fixedEnd, newFixedEnd);
		reference.trim(newFixedStart);
	} else {
		fixed = reference.copy(newFixedStart, newFixedEnd);
		reference.trim(newFixedStart);
	}
	fixedStart = newFixedStart, fixedEnd = newFixedEnd;

	ZAMAN_END(Load);
	
	size_t prevFix = 0;
	uint8_t *len = fixes_loc.data();
	uint8_t *fst = fixes_st.data();
	for (size_t i = 0; i < fixes_replace.size(); i++) {
		int l = *len++;
		if (l == 255) {
			l = getEncoded(fst) + 254 - 1;
		}
		prevFix += l;

		assert(prevFix < fixedEnd);
		assert(prevFix >= fixedStart);
		fixed[prevFix - fixedStart] = fixes_replace.data()[i];
	}

	ZAMAN_END(Sequence);
}

char SequenceDecompressor::operator[] (size_t pos) const
{
	assert(pos >= fixedStart);
	assert(pos < fixedEnd);
	return fixed[pos - fixedStart];
}

void SequenceDecompressor::scanChromosome (const string &s, const SAMComment &samComment)
{
	// by here, all should be fixed ...
	// 	TODO more checking
	//	assert(fixes.size() == 0);
	//	assert(records.size() == 0);
	//	assert(editOperation.size() == 0);

	// clean genomePager
	fixed.resize(0);
	fixedStart = fixedEnd = 0;
	chromosome = reference.scanChromosome(s, samComment);
}
