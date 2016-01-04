#include <thread>
#include <utility>
#include "Sequence.h"
#include "../Streams/rANSOrder0Stream.h"

using namespace std;

//  Fixes     :           14,857,877
//  Replace   :            7,872,718

//FILE *fo;

SequenceCompressor::SequenceCompressor (const string &refFile):
	reference(refFile), 
	fixesLoc(MB, MB),
	fixesReplace(MB, MB),
	fixesLocSt(MB, MB),
	maxEnd(0)
{
	streams.resize(Fields::ENUM_COUNT);
	for (int i = 0; i < streams.size(); i++)
		streams[i] = make_shared<GzipCompressionStream<6>>();
	streams[Fields::FIXES] = make_shared<rANSOrder0CompressionStream<256>>();

	//fo=fopen("_fixLoc", "w");
}

void SequenceCompressor::printDetails(void) 
{
	LOG("  Fixes     : %'20lu", streams[Fields::FIXES]->getCount());
	LOG("  Fixes St  : %'20lu", streams[Fields::FIXES_ST]->getCount());
	LOG("  Replace   : %'20lu", streams[Fields::REPLACE]->getCount());
	//fclose(fo);
}

void SequenceCompressor::updateBoundary (size_t loc) 
{
	maxEnd = max(maxEnd, loc);
}

void SequenceCompressor::outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (chromosome == "*") 
		return;

	ZAMAN_START(SequenceImport);

	out.add((uint8_t*)&fixedStart, sizeof(size_t));
	out.add((uint8_t*)&fixedEnd,   sizeof(size_t));
	out_offset += sizeof(size_t) * 2;
	compressArray(streams[Fields::FIXES], fixesLoc, out, out_offset);
	compressArray(streams[Fields::FIXES_ST], fixesLocSt, out, out_offset);
	compressArray(streams[Fields::REPLACE], fixesReplace, out, out_offset);

	fixesLoc.resize(0);
	fixesLocSt.resize(0);
	fixesReplace.resize(0);

	ZAMAN_END(SequenceImport);
}

char SequenceCompressor::operator[] (size_t pos) const
{
	assert(pos >= fixedStart);
	assert(pos < fixedEnd);
	assert(pos - fixedStart < fixed.size());
	return fixed[pos - fixedStart];
}

void SequenceCompressor::scanChromosome (const string &s, const SAMComment &samComment) 
{
	// by here, all should be fixed ...
	assert(fixesLoc.size() == 0);
	assert(fixesReplace.size() == 0);
	
	// clean genomePager
	fixed.resize(0);
	fixedStart = fixedEnd = maxEnd = 0;

	chromosome = reference.scanChromosome(s, samComment);
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

void SequenceCompressor::applyFixesThread(const Array<Record> &records, const Array<EditOperation> &editOps, Stats &stats,
	size_t fixedStart, size_t offset, size_t size) 
{
	for (size_t k = 0; k < editOps.size(); k++) {
		if (editOps[k].ops[0].first == '*' || records[k].getSequence()[0] == '*') 
			continue;
		if (editOps[k].start >= offset + size)
			break;
		if (editOps[k].end <= offset)
			continue;
		
		size_t genPos = editOps[k].start;
		size_t seqPos = 0;

		for (auto &op: editOps[k].ops) {
			if (genPos >= offset + size) break;
			switch (op.first) {
			case 'M':
			case '=':
			case 'X':
				for (size_t i = 0; genPos < offset + size && i < op.second; i++, genPos++, seqPos++) {
					if (genPos < offset) continue;
					updateGenomeLoc(genPos - fixedStart, records[k].getSequence()[seqPos], stats);
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
}

/*
 * in  nextBlockBegin: we retrieved all reads starting before nextBlockBegin
 * out start_S
 */
size_t SequenceCompressor::applyFixes (size_t nextBlockBegin, const Array<Record> &records, const Array<EditOperation> &editOps,
	size_t &start_S, size_t &end_S, size_t &end_E, size_t &fS, size_t &fE) 
{
	if (editOps.size() == 0) 
		return 0;

	if (chromosome != "*") {
		// Previously, we fixed all locations
		// from fixedStart to fixedEnd
		// New reads locations may overlap fixed locations
		// so fixingStart indicates where should we start
		// actual fixing

		// If we can fix anything
		size_t fixingStart = fixedEnd;
		ZAMAN_START_P(Initialize);
		// Do we have new region to fix?

		if (maxEnd > fixedEnd) {
			// Determine new fixing boundaries
			size_t newFixedEnd = maxEnd;
			size_t newFixedStart = editOps[0].start;
			assert(fixedStart <= newFixedStart);

			// Update fixing table
			if (fixed.size() && newFixedStart < fixedEnd) { // Copy old fixes
				ZAMAN_START_P(Load);
				fixed = fixed.substr(newFixedStart - fixedStart, fixedEnd - newFixedStart);
				fixed += reference.copy(fixedEnd, newFixedEnd);
				reference.trim(fixedEnd);
				ZAMAN_END_P(Load);
			} else {
				ZAMAN_START_P(Load);
				fixed = reference.copy(newFixedStart, newFixedEnd);
				reference.trim(newFixedStart);
				ZAMAN_END_P(Load);
			}

			fixedStart = newFixedStart, fixedEnd = newFixedEnd;
		}
		ZAMAN_END_P(Initialize);

		//	SCREEN("Given boundary is %'lu\n", nextBlockBegin);
		DEBUG("Fixing from %'lu to %'lu\n", fixedStart, fixedEnd);
		//LOG("Reads from %'lu to %'lu\n", records[0].start, records[records.size()-1].end);

		// obtain statistics
		
		ZAMAN_START_P(Calculate);
		vector<thread> t(optThreads);
		#ifdef __SSE2__
			Stats stats(fixedEnd - fixedStart, _mm_setzero_si128());
		#else
			Stats stats(fixedEnd - fixedStart, array<uint16_t, 6>());
		#endif
		size_t maxSz = fixedEnd - fixedStart;
		size_t sz = maxSz / optThreads + 1;
		vector<thread> threads(optThreads);
		for (int i = 0; i < optThreads; i++) 
			threads[i] = thread([&](int ti, size_t start, size_t end) {
					ZAMAN_START(Thread);
					applyFixesThread(records, editOps, stats, fixedStart, start, end);
					ZAMAN_END(Thread);
					ZAMAN_THREAD_JOIN();
					// LOG("Done with %d.. %d %d fixes", ti, start, end);
				}, i, fixedStart + i * sz, min(maxSz, fixedStart + (i + 1) * sz)
			);
		for (int i = 0; i < optThreads; i++) 
			threads[i].join();
		ZAMAN_END_P(Calculate); 


		// patch reference genome
		size_t fixedPrev = 0;
		ZAMAN_START_P(Apply);
		fixesLoc.resize(0);
		fixesLocSt.resize(0);
		fixesReplace.resize(0);

		__m128i invert = _mm_set_epi16(0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff);
		for (size_t i = 0; i < fixedEnd - fixedStart; i++) {
			#ifdef __SSE2__
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
				int p = fixedStart + i - fixedPrev;
				if (p >= 254) {
					fixesLoc.add(255);
					addEncoded(p - 254 + 1, fixesLocSt);
				} else {
					fixesLoc.add(p);
				}
				// addEncoded(fixedStart + i - fixedPrev + 1, fixesLoc);  // +1 for 0 termninator avoid
				// fprintf(fo, "%d %c%c\n", fixedStart + i - fixedPrev + 1, fixed[i], pos[".ACGTN"]);
				fixedPrev = fixedStart + i;
				fixesReplace.add(fixed[i] = pos[".ACGTN"]);
			}
		}

		ZAMAN_END_P(Apply);
	}

	// generate new cigars
	size_t bound;
	for (bound = 0; bound < editOps.size() && editOps[bound].end <= fixedEnd; bound++);
	start_S = editOps[0].start;
	end_S = editOps[bound - 1].start;
	end_E = editOps[bound - 1].end;

	fS = fixedStart;
	fE = fixedEnd;
	//editOperation.setFixed(fixed.data(), fixedStart);

	return bound;
}
