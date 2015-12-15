#include <thread>
#include <utility>
#include "Sequence.h"
using namespace std;

SequenceDecompressor::SequenceDecompressor (const string &refFile, int bs):
	reference(refFile), 
	chromosome("")
{
	fixesStream = new GzipDecompressionStream();
	fixesReplaceStream = new GzipDecompressionStream();
}

SequenceDecompressor::~SequenceDecompressor (void)
{
	delete fixesStream;
	delete fixesReplaceStream;
}

bool SequenceDecompressor::hasRecord (void) 
{
	return true;
}

void SequenceDecompressor::importRecords (uint8_t *in, size_t in_size)  
{
	if (chromosome == "*" || in_size == 0)
		return;

	ZAMAN_START(Decompress_Sequence);

	size_t newFixedStart = *(size_t*)in; in += sizeof(size_t);
	size_t newFixedEnd   = *(size_t*)in; in += sizeof(size_t);
	
	Array<uint8_t> fixes_loc;
	decompressArray(fixesStream, in, fixes_loc);
	Array<uint8_t> fixes_replace;
	decompressArray(fixesReplaceStream, in, fixes_replace);

	ZAMAN_START(Decompress_Sequence_Load);

	assert(fixedStart <= newFixedStart);	
	if (newFixedStart < fixedEnd) { // Copy old fixes
		fixed = fixed.substr(newFixedStart - fixedStart, fixedEnd - newFixedStart);
		fixed += reference.copy(fixedEnd, newFixedEnd);
		reference.trim(fixedEnd);
	} else {
		fixed = reference.copy(newFixedStart, newFixedEnd);
		reference.trim(newFixedStart);
	}
	fixedStart = newFixedStart, fixedEnd = newFixedEnd;

	ZAMAN_END(Decompress_Sequence_Load);
	
	size_t prevFix = 0;
	uint8_t *len = fixes_loc.data();
	for (size_t i = 0; i < fixes_replace.size(); i++) {
		prevFix += getEncoded(len) - 1;
		assert(prevFix < fixedEnd);
		assert(prevFix >= fixedStart);
		fixed[prevFix - fixedStart] = fixes_replace.data()[i];
	}

	ZAMAN_END(Decompress_Sequence);
}

char SequenceDecompressor::operator[] (size_t pos) const
{
	assert(pos >= fixedStart);
	assert(pos < fixedEnd);
	return fixed[pos - fixedStart];
}

void SequenceDecompressor::scanChromosome (const string &s)
{
	ZAMAN_START(Decompress_Sequence_ScanChromosome);

	// by here, all should be fixed ...
	// 	TODO more checking
	//	assert(fixes.size() == 0);
	//	assert(records.size() == 0);
	//	assert(editOperation.size() == 0);

	// clean genomePager
	fixedStart = fixedEnd = 0;
	chromosome = reference.scanChromosome(s);

	ZAMAN_END(Decompress_Sequence_ScanChromosome);
}
