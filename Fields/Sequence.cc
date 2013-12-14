#include "Sequence.h"
using namespace std;

SequenceCompressor::SequenceCompressor (const string &refFile, int bs):
	reference(refFile), 
	fixes_loc(MB, MB),
	fixes_replace(MB, MB),
	fixed(0),
	chromosome("") // first call to scanNextChr will populate this one
{
	fixesStream = new GzipCompressionStream<6>();
	fixesReplaceStream = new GzipCompressionStream<6>();
}

SequenceCompressor::~SequenceCompressor (void) {
	delete[] fixed;
	delete fixesStream;
	delete fixesReplaceStream;
}

void SequenceCompressor::updateBoundary (size_t loc) {
	maxEnd = max(maxEnd, loc);
}

void SequenceCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (chromosome != "*") {	
		out.add((uint8_t*)&fixedStart, sizeof(size_t));
		out.add((uint8_t*)&fixedEnd,   sizeof(size_t));
		out_offset += sizeof(size_t) * 2;
		compressArray(fixesStream, fixes_loc, out, out_offset);
		compressArray(fixesReplaceStream, fixes_replace, out, out_offset);
	
		fixes_loc.resize(0);
		fixes_replace.resize(0);
	}
}

void SequenceCompressor::scanChromosome (const string &s) {
	// by here, all should be fixed ...
	assert(fixes_loc.size() == 0);
	assert(fixes_replace.size() == 0);
	
	// clean genomePager
	delete[] fixed;
	fixed = 0;
	fixedStart = fixedEnd = maxEnd = 0;

	chromosome = reference.scanChromosome(s);
}

// called at the end of the block!
inline void SequenceCompressor::updateGenomeLoc (size_t loc, char ch, Array<int*> &stats) {
	assert(loc < stats.size());
	if (!stats.data()[loc]) {
		stats.data()[loc] = new int[6];
		memset(stats.data()[loc], 0, sizeof(int) * 6);
	}
	stats.data()[loc][getDNAValue(ch)]++;
}

size_t SequenceCompressor::applyFixes (size_t nextBlockBegin, EditOperationCompressor &editOperation,
	size_t &start_S, size_t &end_S, size_t &end_E, size_t &fS, size_t &fE) {
	/////////////////////////////////////////////////////////////////////////////
	if (editOperation.size() == 0) 
		return 0;

	/*    
	prev fixed 			**************
	cur reads start  			##########################      [fully covered positions]
	cur outblock         		##############
	new fixed                     	  *******************
	*/
	// fully covered
	// [records[0].start, records[last].start)
	// previously fixed
	// fixedGenome: [fixedStart, fixedEnd)
	// if we have more unfixed location
	if (editOperation[0].end >= nextBlockBegin)
		return 0;

	if (chromosome != "*") {
		// Previously, we fixed all locations
		// from fixedStart to fixedEnd
		// New reads locations may overlap fixed locations
		// so fixingStart indicates where should we start
		// actual fixing
		size_t fixingStart = fixedEnd;
		if (nextBlockBegin > fixedEnd) {
			size_t newFixedEnd   = nextBlockBegin;
			if (newFixedEnd == (size_t)-1) // chromosome end, fix everything
				newFixedEnd = maxEnd; // records[records.size() - 1].end;
			size_t newFixedStart = editOperation[0].start;
			assert(fixedStart <= newFixedStart);

			char *newFixed = new char[newFixedEnd - newFixedStart];
			// copy old fixes!
			if (fixed && newFixedStart < fixedEnd) {
				memcpy(newFixed, fixed + (newFixedStart - fixedStart), 
					fixedEnd - newFixedStart);
				reference.load(newFixed + (fixedEnd - newFixedStart), 
					fixedEnd, newFixedEnd);
			}
			else reference.load(newFixed, newFixedStart, newFixedEnd);

			fixedStart = newFixedStart;
			fixedEnd = newFixedEnd;
			delete[] fixed;
			fixed = newFixed;
		}

		//	SCREEN("Given boundary is %'lu\n", nextBlockBegin);
		//	SCREEN("Fixing from %'lu to %'lu\n", fixedStart, fixedEnd);
		//	SCREEN("Reads from %'lu to %'lu\n", records[0].start, records[records.size()-1].end);

		// obtain statistics
		Array<int*> stats(0, MB); 
		stats.resize(fixedEnd - fixedStart);
		memset(stats.data(), 0, stats.size() * sizeof(int*));
		for (size_t k = 0; k < editOperation.size(); k++) {
			if (editOperation[k].op == "*") 
				continue;
			size_t size = 0;
			size_t genPos = editOperation[k].start;
			size_t seqPos = 0;
			for (size_t pos = 0; genPos < fixedEnd && pos < editOperation[k].op.length(); pos++) {
				if (isdigit(editOperation[k].op[pos])) {
					size = size * 10 + (editOperation[k].op[pos] - '0');
					continue;
				}
				switch (editOperation[k].op[pos]) {
					case 'M':
					case '=':
					case 'X':
						for (size_t i = 0; genPos < fixedEnd && i < size; i++)
							if (genPos >= fixingStart)
								updateGenomeLoc(genPos++ - fixedStart, editOperation[k].seq[seqPos++], stats);
						break;
					case 'I':
					case 'S':
						seqPos += size;
						break;
					case 'D':
					case 'N':
						for (size_t i = 0; genPos < fixedEnd && i < size; i++)
							if (genPos >= fixingStart)
								updateGenomeLoc(genPos++ - fixedStart, '.', stats);
						break;
				}
				size = 0;
			}
		}

		// patch reference genome
		size_t fixedPrev = 0;
		fixes_loc.resize(0);
		fixes_replace.resize(0);
		for (size_t i = 0; i < fixedEnd - fixedStart; i++) if (stats.data()[i]) {
			int max = -1, pos = -1;
			for (int j = 1; j < 6; j++)
				if (stats.data()[i][j] > max)
					max = stats.data()[i][pos = j];
			if (fixed[i] != pos[".ACGTN"]) {
				//LOG("$FIX %c -> %c %lu %lu", fixed[i], pos[".ACGTN"],
				//	fixedStart + i, fixedStart + i - fixedPrev);

				size_t toadd = fixedStart + i - fixedPrev + 1;
				if (toadd < (1 << 8))
					fixes_loc.add(toadd);
				else if (toadd < (1 << 16)) {
					fixes_loc.add(0);
					fixes_loc.add((toadd >> 8) & 0xff);
					fixes_loc.add(toadd & 0xff);
				}
				else {
					REPEAT(2) fixes_loc.add(0);
					fixes_loc.add((toadd >> 24) & 0xff);
					fixes_loc.add((toadd >> 16) & 0xff);
					fixes_loc.add((toadd >> 8) & 0xff);
					fixes_loc.add(toadd & 0xff);
				}
				// fixes_loc.add();
				fixedPrev = fixedStart + i;
				fixes_replace.add(fixed[i] = pos[".ACGTN"]);
				
			}
			delete[] stats.data()[i];
		}
	}

	// generate new cigars
	size_t bound;
	for (bound = 0; bound < editOperation.size() && editOperation[bound].end <= fixedEnd; bound++);
	start_S = editOperation[0].start;
	end_S = editOperation[bound-1].start;
	end_E = editOperation[bound-1].end;
	fS = fixedStart;
	fE = fixedEnd;
	editOperation.setFixed(fixed, fixedStart);
	return bound;
}

/************************************************************************************************************************/

SequenceDecompressor::SequenceDecompressor (const string &refFile, int bs):
	reference(refFile), 
	chromosome(""),
	fixed(0)
{
	fixesStream = new GzipDecompressionStream();
	fixesReplaceStream = new GzipDecompressionStream();
}

SequenceDecompressor::~SequenceDecompressor (void) {
	delete[] fixed;
	delete fixesStream;
	delete fixesReplaceStream;
}


bool SequenceDecompressor::hasRecord (void) {
	return true;
}

void SequenceDecompressor::importRecords (uint8_t *in, size_t in_size)  {
	if (chromosome == "*" || in_size == 0)
		return;

	size_t newFixedStart = *(size_t*)in; in += sizeof(size_t);
	size_t newFixedEnd   = *(size_t*)in; in += sizeof(size_t);
	//size_t readsStart    = *(size_t*)in; in += sizeof(size_t);

	Array<uint8_t> fixes_loc;
	decompressArray(fixesStream, in, fixes_loc);
	Array<uint8_t> fixes_replace;
	decompressArray(fixesReplaceStream, in, fixes_replace);

	////////////
	// now move
	// NEED FROM READSSTART
	assert(fixedStart <= newFixedStart);	
	char *newFixed = new char[newFixedEnd - newFixedStart];
	// copy old fixes!
	if (fixed && newFixedStart < fixedEnd) {
		memcpy(newFixed, fixed + (newFixedStart - fixedStart), 
			fixedEnd - newFixedStart);
		reference.load(newFixed + (fixedEnd - newFixedStart), 
			fixedEnd, newFixedEnd);
	}
	else reference.load(newFixed, newFixedStart, newFixedEnd);

	fixedStart = newFixedStart;
	fixedEnd = newFixedEnd;
	delete[] fixed;
	fixed = newFixed;
	
	size_t prevFix = 0;
	uint8_t *len = fixes_loc.data();
	for (size_t i = 0; i < fixes_replace.size(); i++) {
		int T = 1;
		if (!*len) T = 2, len++;
		if (!*len) T = 4, len++;
		size_t fl = 0;
		REPEAT(T) fl |= *len++ << (8 * (T - _ - 1));
		assert(fl > 0);
		prevFix += --fl;

		assert(prevFix < fixedEnd);
		assert(prevFix >= fixedStart);
		fixed[prevFix - fixedStart] = fixes_replace.data()[i];
	}
}

void SequenceDecompressor::setFixed (EditOperationDecompressor &editOperation) {
	editOperation.setFixed(fixed, fixedStart);
}

void SequenceDecompressor::scanChromosome (const string &s) {
	// by here, all should be fixed ...
	// 	TODO more checking
	//	assert(fixes.size() == 0);
	//	assert(records.size() == 0);
	//	assert(editOperation.size() == 0);

	// clean genomePager
	delete[] fixed;
	fixed = 0;
	fixedStart = fixedEnd = 0;
	chromosome = reference.scanChromosome(s);
}
