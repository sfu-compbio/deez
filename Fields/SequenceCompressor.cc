#include <thread>
#include <utility>
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

SequenceCompressor::~SequenceCompressor (void) 
{
	delete[] fixed;
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
	compressArray(fixesStream, fixes_loc, out, out_offset);
	compressArray(fixesReplaceStream, fixes_replace, out, out_offset);

	fixes_loc.resize(0);
	fixes_replace.resize(0);

	ZAMAN_END(Compress_Sequence);
}

void SequenceCompressor::scanChromosome (const string &s) 
{
	ZAMAN_START(Compress_Sequence_ScanChromosome);

	// by here, all should be fixed ...
	assert(fixes_loc.size() == 0);
	assert(fixes_replace.size() == 0);
	
	// clean genomePager
	delete[] fixed;
	fixed = 0;
	fixedStart = fixedEnd = maxEnd = 0;

	chromosome = reference.scanChromosome(s);

	ZAMAN_END(Compress_Sequence_ScanChromosome);
}

// called at the end of the block!
// IS NOT ATOMIC!
inline void SequenceCompressor::updateGenomeLoc (size_t loc, char ch, Array<int*> &stats) 
{
	assert(loc < stats.size());
	if (!stats.data()[loc]) {
		stats.data()[loc] = new int[6];
		memset(stats.data()[loc], 0, sizeof(int) * 6);
	}
	stats.data()[loc][getDNAValue(ch)]++;
}

std::mutex SequenceCompressor::mtx;
void SequenceCompressor::applyFixesThread(EditOperationCompressor &editOperation, Array<int*> &stats, 
	size_t fixedStart, size_t offset, size_t size, const char *fixed) 
{
	ZAMAN_START(Compress_Sequence_Fixes_ApplyThread);

	for (size_t k = 0; k < editOperation.size(); k++) {
		if (editOperation[k].op == "*" || editOperation[k].seq == "*") 
			continue;
		if (editOperation[k].start >= offset + size)
			break;
		if (editOperation[k].end <= offset)
			continue;
		
		size_t genPos = editOperation[k].start;
		size_t num = 0;
		size_t seqPos = 0;

		size_t mdOperLen = 0;
		string mdOperTag;
		int NM = 0;

		for (size_t pos = 0; genPos < offset + size && pos < editOperation[k].op.length(); pos++) {
			if (isdigit(editOperation[k].op[pos])) {
				num = num * 10 + (editOperation[k].op[pos] - '0');
				continue;
			}
			switch (editOperation[k].op[pos]) {
				case 'M':
				case '=':
				case 'X':
					for (size_t i = 0; genPos < offset + size && i < num; i++, genPos++, seqPos++) {
						if (genPos >= offset) {
							updateGenomeLoc(genPos - fixedStart, editOperation[k].seq[seqPos], stats);

							if (fixed[genPos - fixedStart] != editOperation[k].seq[seqPos]) {
								mdOperTag += inttostr(mdOperLen), mdOperLen = 0;
								mdOperTag += fixed[genPos - fixedStart];
								NM++;
							} else {
								mdOperLen++;
							}
						}
					}
					break;
				case 'D':
					if (genPos + num < size) {
						mdOperTag += inttostr(mdOperLen), mdOperLen = 0;
						mdOperTag += "^";
						mdOperTag += string(fixed + genPos - fixedStart, num); 
					}
					NM += num;
				case 'N':
					for (size_t i = 0; genPos < offset + size && i < num; i++, genPos++) {
						if (genPos >= offset)
							updateGenomeLoc(genPos - fixedStart, '.', stats);
					}
					break;
				case 'I':
					NM += num;
				case 'S':
					seqPos += num; 
					break;
			}
			num = 0;
		}
		if (mdOperLen || !isdigit(mdOperTag.back())) mdOperTag += inttostr(mdOperLen);
		if (!isdigit(mdOperTag[0])) mdOperTag = "0" + mdOperTag;

		mtx.lock();
		editOperation[k].MD = mdOperTag;
		editOperation[k].NM = NM;
		mtx.unlock();
	}

	ZAMAN_END(Compress_Sequence_Fixes_ApplyThread);
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
			Array<char> newFixed(newFixedEnd - newFixedStart);
			newFixed.resize(newFixed.capacity());
			//char *newFixed = new char[newFixedEnd - newFixedStart];
			// copy old fixes!
			if (fixed && newFixedStart < fixedEnd) {
				memcpy(newFixed.data(), fixed.data() + (newFixedStart - fixedStart), 
					fixedEnd - newFixedStart);
				ZAMAN_START(Compress_Sequence_Fixes_Initialize_Load);
				reference.load(newFixed.data() + (fixedEnd - newFixedStart), 
					fixedEnd, newFixedEnd);
				ZAMAN_END(Compress_Sequence_Fixes_Initialize_Load);
			} else {
				ZAMAN_START(Compress_Sequence_Fixes_Initialize_Load);
				reference.load(newFixed.data(), newFixedStart, newFixedEnd);
				ZAMAN_END(Compress_Sequence_Fixes_Initialize_Load);
			}

			fixedStart = newFixedStart, fixedEnd = newFixedEnd;
			fixed = newFixed;
		}
		ZAMAN_END(Compress_Sequence_Fixes_Initialize);

		//	SCREEN("Given boundary is %'lu\n", nextBlockBegin);
		//	SCREEN("Fixing from %'lu to %'lu\n", fixedStart, fixedEnd);
		//	SCREEN("Reads from %'lu to %'lu\n", records[0].start, records[records.size()-1].end);

		// obtain statistics

		
		ZAMAN_START(Compress_Sequence_Fixes_Calculate);
		Array<int*> stats(0, MB); 
		stats.resize(fixedEnd - fixedStart);
		memset(stats.data(), 0, stats.size() * sizeof(int*));

		vector<std::thread> t;
		size_t sz = stats.size() / optThreads + 1;
		for (int i = 0; i < optThreads; i++)
			t.push_back(thread(applyFixesThread, 
				ref(editOperation), ref(stats), fixedStart, 
				fixedStart + i * sz, min(sz, stats.size() - i * sz),
				fixed
			));
		for (int i = 0; i < optThreads; i++)
			t[i].join();
		ZAMAN_END(Compress_Sequence_Fixes_Calculate); 


		// patch reference genome
		size_t fixedPrev = 0;
		ZAMAN_START(Compress_Sequence_Fixes_Apply);
		fixes_loc.resize(0);
		fixes_replace.resize(0);
		for (size_t i = 0; i < fixedEnd - fixedStart; i++) if (stats.data()[i]) {
			int max = -1, pos = -1;
			for (int j = 1; j < 6; j++) {
				if (stats.data()[i][j] > max)
					max = stats.data()[i][pos = j];
			}
			if (fixed[i] != pos[".ACGTN"]) 
			{
				// +1 for 0 termninator avoid
				addEncoded(fixedStart + i - fixedPrev + 1, fixes_loc);
				fixedPrev = fixedStart + i;
				fixes_replace.add(fixed[i] = pos[".ACGTN"]);
			}
			delete[] stats.data()[i];
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
	editOperation.setFixed(fixed, fixedStart);

	ZAMAN_END(Compress_Sequence_Fixes);

	return bound;
}
