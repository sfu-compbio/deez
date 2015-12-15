#include <thread>
#include <utility>
#include "Sequence.h"
using namespace std;

SequenceCompressor::SequenceCompressor (const string &refFile, int bs):
	reference(refFile), 
	fixesLoc(MB, MB),
	fixesReplace(MB, MB)
{
	fixesStream = new GzipCompressionStream<6>();
	fixesReplaceStream = new GzipCompressionStream<6>();
}

SequenceCompressor::~SequenceCompressor (void) 
{
	delete fixesStream;
	delete fixesReplaceStream;
}

size_t SequenceCompressor::compressedSize(void) 
{ 
	return fixesStream->getCount() + fixesReplaceStream->getCount(); 
}

void SequenceCompressor::printDetails(void) 
{
	LOG("  Fixes     : %'20lu", fixesStream->getCount());
	LOG("  Replace   : %'20lu", fixesReplaceStream->getCount());
}

void SequenceCompressor::updateBoundary (size_t loc) 
{
	maxEnd = max(maxEnd, loc);
}

void SequenceCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (chromosome == "*") 
		return;

	ZAMAN_START(Compress_Sequence);

	out.add((uint8_t*)&fixedStart, sizeof(size_t));
	out.add((uint8_t*)&fixedEnd,   sizeof(size_t));
	out_offset += sizeof(size_t) * 2;
	compressArray(fixesStream, fixesLoc, out, out_offset);
	compressArray(fixesReplaceStream, fixesReplace, out, out_offset);

	fixesLoc.resize(0);
	fixesReplace.resize(0);

	ZAMAN_END(Compress_Sequence);
}

char SequenceCompressor::operator[] (size_t pos) const
{
	assert(pos >= fixedStart);
	assert(pos < fixedEnd);
	assert(pos - fixedStart < fixed.size());
	return fixed[pos - fixedStart];
}

void SequenceCompressor::scanChromosome (const string &s) 
{
	ZAMAN_START(Compress_Sequence_ScanChromosome);

	// by here, all should be fixed ...
	assert(fixesLoc.size() == 0);
	assert(fixesReplace.size() == 0);
	
	// clean genomePager
	fixed.resize(0);
	fixedStart = fixedEnd = maxEnd = 0;

	chromosome = reference.scanChromosome(s);

	ZAMAN_END(Compress_Sequence_ScanChromosome);
}

// called at the end of the block!
// IS NOT ATOMIC!

#define _ 0
static __m128i getSSEvalueCache[] =  {
	_mm_set_epi16(_,_,_,_,_,_,_,1),
	_mm_set_epi16(_,_,_,_,_,_,1,_),
	_mm_set_epi16(_,_,_,_,_,1,_,_),
	_mm_set_epi16(_,_,_,_,1,_,_,_),
	_mm_set_epi16(_,_,_,1,_,_,_,_),
	_mm_set_epi16(_,_,1,_,_,_,_,_),
	_mm_set_epi16(_,1,_,_,_,_,_,_),
	_mm_set_epi16(1,_,_,_,_,_,_,_),
};
#undef _
inline void SequenceCompressor::updateGenomeLoc (size_t loc, char ch, Stats &stats) 
{
	#ifdef __SSE2__
		stats[loc] = _mm_add_epi16(stats[loc], getSSEvalueCache[getDNAValue(ch)]);
	#else
		stats[loc][getDNAValue(ch)]++;
	#endif
}

void SequenceCompressor::applyFixesThread(EditOperationCompressor &editOperation, Stats &stats,
	size_t fixedStart, size_t offset, size_t size) 
{
	ZAMAN_START(Compress_Sequence_Fixes_CalculateThread);

	for (size_t k = 0; k < editOperation.size(); k++) {
		if (editOperation[k].op == "*" || editOperation[k].seq == "*") 
			continue;
		if (editOperation[k].start >= offset + size)
			break;
		if (editOperation[k].end <= offset)
			continue;
		
		size_t genPos = editOperation[k].start;
		size_t seqPos = 0;

		for (int opi = 0; opi < editOperation[k].ops.size(); opi++) {
			auto &op = editOperation[k].ops[opi];
			if (genPos >= offset + size) break;
			switch (op.first) {
			case 'M':
			case '=':
			case 'X':
				for (size_t i = 0; genPos < offset + size && i < op.second; i++, genPos++, seqPos++) {
					if (genPos < offset) continue;
					updateGenomeLoc(genPos - fixedStart, editOperation[k].seq[seqPos], stats);
				}
				break;
			case 'D':
			//case 'N':
				for (size_t i = 0; genPos < offset + size && i < op.second; i++, genPos++) {
					if (genPos < offset) continue;
					updateGenomeLoc(genPos - fixedStart, '.', stats);
				}
				break;
			case 'I':
			case 'S':
				seqPos += op.second; 
				break;
			}
		}
	}
	ZAMAN_END(Compress_Sequence_Fixes_CalculateThread);
}

/*
 * in  nextBlockBegin: we retrieved all reads starting before nextBlockBegin
 * out start_S
 */
size_t SequenceCompressor::applyFixes (size_t nextBlockBegin, EditOperationCompressor &editOperation,
	size_t &start_S, size_t &end_S, size_t &end_E, size_t &fS, size_t &fE) 
{
	if (editOperation.size() == 0) 
		return 0;

	ZAMAN_START(Compress_Sequence_Fixes);

	if (chromosome != "*") {
		// Previously, we fixed all locations
		// from fixedStart to fixedEnd
		// New reads locations may overlap fixed locations
		// so fixingStart indicates where should we start
		// actual fixing

		// If we can fix anything
		size_t fixingStart = fixedEnd;
		ZAMAN_START(Compress_Sequence_Fixes_Initialize);
		// Do we have new region to fix?

		if (maxEnd > fixedEnd) {
			// Determine new fixing boundaries
			size_t newFixedEnd = maxEnd;
			size_t newFixedStart = editOperation[0].start;
			assert(fixedStart <= newFixedStart);

			// Update fixing table
			if (fixed.size() && newFixedStart < fixedEnd) { // Copy old fixes
				ZAMAN_START(Compress_Sequence_Fixes_Initialize_Load);
				fixed = fixed.substr(newFixedStart - fixedStart, fixedEnd - newFixedStart);
				fixed += reference.copy(fixedEnd, newFixedEnd);
				reference.trim(fixedEnd);
				ZAMAN_END(Compress_Sequence_Fixes_Initialize_Load);
			} else {
				ZAMAN_START(Compress_Sequence_Fixes_Initialize_Load);
				fixed = reference.copy(newFixedStart, newFixedEnd);
				reference.trim(newFixedStart);
				ZAMAN_END(Compress_Sequence_Fixes_Initialize_Load);
			}

			fixedStart = newFixedStart, fixedEnd = newFixedEnd;
		}
		ZAMAN_END(Compress_Sequence_Fixes_Initialize);

		//	SCREEN("Given boundary is %'lu\n", nextBlockBegin);
		DEBUG("Fixing from %'lu to %'lu\n", fixedStart, fixedEnd);
		//LOG("Reads from %'lu to %'lu\n", records[0].start, records[records.size()-1].end);

		// obtain statistics
		
		ZAMAN_START(Compress_Sequence_Fixes_Calculate);
		vector<thread> t(optThreads);
		#ifdef __SSE2__
			Stats stats(fixedEnd - fixedStart, _mm_setzero_si128());
		#else
			Stats stats(fixedEnd - fixedStart, array<uint16_t, 6>());
		#endif
		size_t maxSz = fixedEnd - fixedStart;
		size_t sz = maxSz / optThreads + 1;
		for (int i = 0; i < optThreads; i++) {
			t[i] = thread(applyFixesThread, 
				ref(editOperation), ref(stats), fixedStart, 
				fixedStart + i * sz, min(sz, maxSz - i * sz)
			);
		}
		for (int i = 0; i < optThreads; i++)
			t[i].join();
		ZAMAN_END(Compress_Sequence_Fixes_Calculate); 


		// patch reference genome
		size_t fixedPrev = 0;
		ZAMAN_START(Compress_Sequence_Fixes_Apply);
		fixesLoc.resize(0);
		fixesReplace.resize(0);


		__m128i invert = _mm_set_epi16(0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff);
		for (size_t i = 0; i < fixedEnd - fixedStart; i++) {
			#ifdef __SSE2__
				#warning "using sse2"
				if (_mm_movemask_epi8(_mm_cmpeq_epi8(stats[i], _mm_setzero_si128())) == 0xFFFF)
					continue;
				int pos = _mm_extract_epi16(_mm_minpos_epu16(_mm_sub_epi16(invert, stats[i])), 1);
			#else
				int max = -1, pos = -1;
				for (int j = 1; j < 6; j++) {
					if (stats[i][j] > max)
						max = stats[i][pos = j];
				}
				if (max <= 0) continue;
			#endif

			if (fixed[i] != pos[".ACGTN"]) {
				addEncoded(fixedStart + i - fixedPrev + 1, fixesLoc);  // +1 for 0 termninator avoid
				fixedPrev = fixedStart + i;
				fixesReplace.add(fixed[i] = pos[".ACGTN"]);
			}
		}
		ZAMAN_END(Compress_Sequence_Fixes_Apply);
	}

	// generate new cigars
	size_t bound;
	for (bound = 0; bound < editOperation.size() && editOperation[bound].end <= fixedEnd; bound++);
	start_S = editOperation[0].start;
	end_S = editOperation[bound-1].start;
	end_E = editOperation[bound-1].end;

	fS = fixedStart;
	fE = fixedEnd;
	//editOperation.setFixed(fixed.data(), fixedStart);

	ZAMAN_END(Compress_Sequence_Fixes);

	return bound;
}
