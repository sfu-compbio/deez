#include <thread>
#include <utility>
#include "Sequence.h"
using namespace std;

SequenceDecompressor::SequenceDecompressor (const string &refFile, int bs):
	reference(refFile), 
	chromosome(""),
	fixed(0), ref(0)
{
	fixesStream = new GzipDecompressionStream();
	fixesReplaceStream = new GzipDecompressionStream();
}

SequenceDecompressor::~SequenceDecompressor (void)
{
	delete[] fixed;
	delete[] ref;
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

	////////////
	// NEED FROM READSSTART

	ZAMAN_START(Decompress_Sequence_Load);

	assert(fixedStart <= newFixedStart);	
	char *newFixed = new char[newFixedEnd - newFixedStart];
	char *newRef = new char[newFixedEnd - newFixedStart];
	// copy old fixes!
	if (fixed && newFixedStart < fixedEnd) {
		memcpy(newFixed, fixed + (newFixedStart - fixedStart), 
			fixedEnd - newFixedStart);
		memcpy(newRef, ref + (newFixedStart - fixedStart), 
			fixedEnd - newFixedStart);
		reference.load(newFixed + (fixedEnd - newFixedStart), 
			fixedEnd, newFixedEnd);
		memcpy(newRef + (fixedEnd - newFixedStart), 
			newFixed + (fixedEnd - newFixedStart), fixedEnd - newFixedStart);
	}
	else {
		reference.load(newFixed, newFixedStart, newFixedEnd);
		memcpy(newRef, newFixed, newFixedEnd - newFixedStart);
	}

	fixedStart = newFixedStart;
	fixedEnd = newFixedEnd;
	delete[] fixed;
	fixed = newFixed;
	delete[] ref;
	ref = newRef;

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

void SequenceDecompressor::setFixed (EditOperationDecompressor &editOperation) 
{
	editOperation.setFixed(fixed, ref, fixedStart);
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
	delete[] fixed;
	delete[] ref;
	fixed = ref = 0;
	fixedStart = fixedEnd = 0;
	chromosome = reference.scanChromosome(s);

	ZAMAN_END(Decompress_Sequence_ScanChromosome);
}
