#include <thread>
#include <utility>
#include "Sequence.h"
#include "../Streams/rANSOrder0Stream.h"

using namespace std;

#define _ 0
const __m128i GenomeStatsSSE::getSSEvalueCache[] = {
	_mm_set_epi16(_,_,_,_,_,_,_,1),
	_mm_set_epi16(_,_,_,_,_,_,1,_),
	_mm_set_epi16(_,_,_,_,_,1,_,_),
	_mm_set_epi16(_,_,_,_,1,_,_,_),
	_mm_set_epi16(_,_,_,1,_,_,_,_),
	_mm_set_epi16(_,_,1,_,_,_,_,_),
	_mm_set_epi16(_,1,_,_,_,_,_,_),
	_mm_set_epi16(1,_,_,_,_,_,_,_)
};
#undef _
const __m128i GenomeStatsSSE::invert = _mm_set_epi16(0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff);

SequenceCompressor::SequenceCompressor (const string &refFile):
	reference(refFile), 
	fixesLoc(MB, MB),
	fixesReplace(MB, MB),
	fixesLocSt(MB, MB),
	maxEnd(0)
{
	if (!__builtin_cpu_supports("sse4.1")) {
		WARN("Warning: SSE4.1 not found -- using sub-optimal sequence compression method.");
	}

	streams.resize(Fields::ENUM_COUNT);
	if (optBzip) {
		for (int i = 0; i < streams.size(); i++)
			streams[i] = make_shared<BzipCompressionStream>();
	} else {
		for (int i = 0; i < streams.size(); i++)
			streams[i] = make_shared<GzipCompressionStream<6, Z_RLE>>();
	}
	streams[Fields::FIXES] = make_shared<rANSOrder0CompressionStream<256>>();
}

void SequenceCompressor::printDetails(void) 
{
	LOG("  Fixes     : %'20lu", streams[Fields::FIXES]->getCount());
	LOG("  Fixes St  : %'20lu", streams[Fields::FIXES_ST]->getCount());
	LOG("  Replace   : %'20lu", streams[Fields::REPLACE]->getCount());
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

void SequenceCompressor::applyFixesThread(const Array<Record> &records, 
	const Array<EditOperation> &editOps, shared_ptr<GenomeStats> &stats,
	size_t fixedStart, size_t start, size_t end) 
{
	for (size_t k = start; k < end; k++) {
		const char *seq = records[k].getSequence();
		if (editOps[k].ops[0].first == '*' || seq[0] == '*') 
			continue;

		size_t genPos = editOps[k].start;
		size_t seqPos = 0;
		for (auto &op: editOps[k].ops) {
			switch (op.first) {
			case 'M':
			case '=':
			case 'X':
				for (size_t i = 0; i < op.second; i++, genPos++, seqPos++) 
					stats->updateGenomeLoc(genPos - fixedStart, seq[seqPos]);
				break;
			case 'D':
				for (size_t i = 0; i < op.second; i++, genPos++) 
					stats->updateGenomeLoc(genPos - fixedStart, '.');
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
size_t SequenceCompressor::applyFixes (size_t nextBlockBegin, 
	const Array<Record> &records, const Array<EditOperation> &editOps,
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
		auto stats = GenomeStatsSSE::newStats(fixedEnd - fixedStart);
		size_t maxSz = editOps.size();
		size_t sz = maxSz / optThreads + 1;
		vector<thread> threads(optThreads);
		mutex mtx;
		mtx.lock(); // lock for stats array
		for (int i = 0; i < optThreads; i++) {
			threads[i] = thread([&](int ti, size_t start, size_t end) {
				ZAMAN_START(Thread);
				if (ti == 0) {
					applyFixesThread(records, editOps, stats, fixedStart, start, end);
					mtx.unlock();
					return;
				}
				if (start >= editOps.size())
					return;

				size_t genomeStart = editOps[start].start;
				size_t genomeEnd = 0;
				for (size_t i = start; i < end; i++) 
					genomeEnd = max(genomeEnd, editOps[i].end);

				auto threadStats = GenomeStatsSSE::newStats(genomeEnd - genomeStart);
				applyFixesThread(records, editOps, threadStats, genomeStart, start, end);
				ZAMAN_END(Thread);

				unique_lock<mutex> lock(mtx);
				for (size_t i = 0; i < threadStats->size(); i++) {
					stats->update(i + genomeStart - fixedStart, threadStats, i);
				}
				ZAMAN_THREAD_JOIN();
				// LOG("Done with %d.. %d %d fixes", ti, start, end);
			}, i, i * sz, min(maxSz, (i + 1) * sz));
		}
		for (int i = 0; i < optThreads; i++) 
			threads[i].join();
		ZAMAN_END_P(Calculate); 

		// patch reference genome
		size_t fixedPrev = 0;
		ZAMAN_START_P(Apply);
		fixesLoc.resize(0);
		fixesLocSt.resize(0);
		fixesReplace.resize(0);

		for (size_t i = 0; i < fixedEnd - fixedStart; i++) {
			int pos = stats->maxPos(i);
			if (pos == -1)
				continue;
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
