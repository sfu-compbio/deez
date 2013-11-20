#include "Sequence.h"
using namespace std;

inline string int2str (int k) {
	string s = "";
	while (k) {
		s = char(k % 10 + '0') + s;
		k /= 10;
	}
	return s;
}

inline string inttostr (int k) {
    static vector<std::string> mem;
    if (mem.size() == 0) {
    	mem.resize(10001);
		mem[0] = "0";
		for (int i = 1; i < 10000; i++)
			mem[i] = int2str(i);
    }

    if (k < 10000)
    	return mem[k];
    else
    	return int2str(k);
}

inline char getDNAValue (char ch) {
	static char c[128] = {
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 0 15
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 16 31
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 5, // 32 47
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 48 63
		5, 1, 5, 2, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, // 64 79
		5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 80 95
		5, 1, 5, 2, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, // 96 111
		5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 112 127
	};	
	return c[ch];
}

/********************************************************************************/

SequenceCompressor::SequenceCompressor (const string &refFile, int bs):
	reference(refFile), 
	editOperation(bs),
	records(bs),
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

void SequenceCompressor::addRecord (size_t loc, const string &seq, const string &op) {
	EditOP eo;
	eo.start = loc;
	eo.seq = seq;
	eo.op = op;
	eo.end = loc;

	// calculate ending
	size_t size = 0;
	if (op != "*") for (size_t pos = 0; pos < op.length(); pos++) {
		if (isdigit(op[pos])) {
			size = size * 10 + (op[pos] - '0');
			continue;
		}
		switch (op[pos]) {
			case 'M':
			case '=':
			case 'X':
			case 'D':
			case 'N':
				eo.end += size;
				break;
			default:
				break;
		}
		size = 0;
	}
	maxEnd = max(maxEnd, eo.end);
	records.add(eo);
}

void SequenceCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	// fixes
	size_t off = out_offset, s;
	if (chromosome != "*") {	
		out.resize(off);

		out.add((uint8_t*)&fixedStart, sizeof(size_t));
		out.add((uint8_t*)&fixedEnd,   sizeof(size_t));
		//out.add((uint8_t*)&readsStart, sizeof(size_t));
		off += sizeof(size_t) * 2;
		out.resize(off);

	//ZAMAN_START();
		s = 0;
		if (fixes_loc.size()) s = fixesStream->compress((uint8_t*)fixes_loc.data(), 
			fixes_loc.size() * sizeof(uint32_t), out, off + 2 * sizeof(size_t));
		*(size_t*)(out.data() + off) = s;
		*(size_t*)(out.data() + off + sizeof(size_t)) = fixes_loc.size() * sizeof(uint32_t);
		off += s + 2 * sizeof(size_t);		
		out.resize(off);
	//ZAMAN_END("FixesStream")
		
	//ZAMAN_START();
		s = 0;
		if (fixes_replace.size()) s = fixesReplaceStream->compress((uint8_t*)fixes_replace.data(), 
			fixes_replace.size(), out, off + 2 * sizeof(size_t));
		*(size_t*)(out.data() + off) = s;
		*(size_t*)(out.data() + off + sizeof(size_t)) = fixes_replace.size();
		off += s + 2 * sizeof(size_t);		
		out.resize(off);
	//ZAMAN_END("ReplaceStream");
	
		fixes_loc.resize(0);
		fixes_replace.resize(0);
	}

	// operations
	editOperation.outputRecords(out, off, k);
	assert(editOperation.size() == 0);
}

void SequenceCompressor::scanChromosome (const string &s) {
	// by here, all should be fixed ...
	assert(fixes_loc.size() == 0);
	assert(fixes_replace.size() == 0);
	assert(records.size() == 0);
	assert(editOperation.size() == 0);

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
size_t SequenceCompressor::applyFixes (size_t nextBlockBegin, 
	size_t &start_S, size_t &end_S, size_t &end_E, size_t &fS, size_t &fE) {
	/////////////////////////////////////////////////////////////////////////////
	if (records.size() == 0) 
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
	if (records[0].end >= nextBlockBegin)
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
			size_t newFixedStart = records[0].start;
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
		for (size_t k = 0; k < records.size(); k++) {
			if (records[k].op == "*") 
				continue;
			size_t size = 0;
			size_t genPos = records[k].start;
			size_t seqPos = 0;
			for (size_t pos = 0; genPos < fixedEnd && pos < records[k].op.length(); pos++) {
				if (isdigit(records[k].op[pos])) {
					size = size * 10 + (records[k].op[pos] - '0');
					continue;
				}
				switch (records[k].op[pos]) {
					case 'M':
					case '=':
					case 'X':
						for (size_t i = 0; genPos < fixedEnd && i < size; i++)
							if (genPos >= fixingStart)
								updateGenomeLoc(genPos++ - fixedStart, records[k].seq[seqPos++], stats);
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
				fixes_loc.add(fixedStart + i - fixedPrev);
				fixedPrev = fixedStart + i;
				fixes_replace.add(fixed[i] = pos[".ACGTN"]);
			}
			delete[] stats.data()[i];
		}
	}

	// generate new cigars
	size_t bound;
	for (bound = 0; bound < records.size() && records[bound].end <= fixedEnd; bound++)
		editOperation.addRecord(getEditOP(records[bound].start - fixedStart, records[bound].seq, records[bound].op));
	assert(bound);
	start_S = records[0].start;
	end_S 	= records[bound-1].start;
	end_E 	= records[bound-1].end;
	fS = fixedStart;
	fE = fixedEnd;
	records.remove_first_n(bound);
	return bound;
}

string SequenceCompressor::getEditOP(size_t loc, const string &seq, const string &op) {
	if (op == "*") 
		return seq + "*";

	string newOP;

	size_t size   = 0;
	size_t genPos = loc;
	size_t seqPos = 0;

	char lastOP = 0;
	int  lastOPSize = 0;

	for (size_t pos = 0; pos < op.size(); pos++) {
		if (isdigit(op[pos])) {
			size = size * 10 + (op[pos] - '0');
			continue;
		}

		lastOP = 0;
		lastOPSize = 0;
		switch (op[pos]) {
			case 'M':
			case '=':
			case 'X':
				for (size_t i = 0; i < size; i++) {
					if (seq[seqPos] == fixed[genPos]) {
						if (lastOP == 0) {
							lastOP = '=';
							lastOPSize = 1;
						}
						else if (lastOP == '=')
							lastOPSize++;
						else if (lastOP != 0) {
							if (lastOP != 'X') 
								newOP += inttostr(lastOPSize) + lastOP;
							lastOP = '=';
							lastOPSize = 1;
						}
					}
					else if (lastOP == 'X')
						newOP += tolower(seq[seqPos]);							
					else {
						if (lastOP)
							newOP += inttostr(lastOPSize) + lastOP + (char)tolower(seq[seqPos]);
						else
							newOP += (char)tolower(seq[seqPos]);
						lastOP = 'X';
						lastOPSize =0;
					}
					genPos++;
					seqPos++;
				}
				if (lastOP && lastOP != 'X')
					newOP += inttostr(lastOPSize) + lastOP;
				break;
			case 'I':
			case 'S':
				for (size_t i = 0; i < size; i++)
					newOP += seq[seqPos + i];
				newOP  += op[pos];
				seqPos += size;
				break;
			case 'D':
			case 'N':
				if (op[pos] == 'N')
					newOP += inttostr(size) + 'K';
				else
					newOP += inttostr(size) + op[pos];
				genPos += size;
				break;
		}
		size = 0;
	}
	return newOP;
}

/************************************************************************************************************************/

SequenceDecompressor::SequenceDecompressor (const string &refFile, int bs):
	reference(refFile), 
	editOperation(bs),
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
	return editOperation.hasRecord();
}

EditOP SequenceDecompressor::getRecord (size_t loc) {
	return getSeqCigar(loc, editOperation.getRecord());
}

size_t SequenceDecompressor::importFixes (uint8_t *in, size_t in_size) {
	if (chromosome == "*" || in_size == 0)
		return 0;

	size_t newFixedStart = *(size_t*)in; in += sizeof(size_t);
	size_t newFixedEnd   = *(size_t*)in; in += sizeof(size_t);
	//size_t readsStart    = *(size_t*)in; in += sizeof(size_t);

	size_t in1 = *(size_t*)in; in += sizeof(size_t);
	size_t de1 = *(size_t*)in; in += sizeof(size_t);
	Array<uint8_t> fixes_loc;
	fixes_loc.resize(de1);
	size_t s1 = 0;
	if (in1) s1 = fixesStream->decompress(in, in1, fixes_loc, 0);
	assert(s1 == de1);
	in += in1;

	size_t in2 = *(size_t*)in; in += sizeof(size_t);
	size_t de2 = *(size_t*)in; in += sizeof(size_t);
	Array<uint8_t> fixes_replace;
	fixes_replace.resize(de2);
	size_t s2 = 0;
	if (in2) s2 = fixesReplaceStream->decompress(in, in2, fixes_replace, 0);
	assert(s2 == de2);
	in += in2;
	assert(fixes_loc.size() == fixes_replace.size() * sizeof(uint32_t));

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
	for (size_t i = 0; i < fixes_replace.size(); i++) {
		prevFix += *((uint32_t*)fixes_loc.data() + i);
		assert(prevFix < fixedEnd);
		assert(prevFix >= fixedStart);
		fixed[prevFix - fixedStart] = fixes_replace.data()[i];
	}

	return 6 * sizeof(size_t) + in1 + in2;
}

void SequenceDecompressor::importRecords (uint8_t *in, size_t in_size) {
	size_t off = importFixes(in, in_size);
	editOperation.importRecords(in + off, in_size - off);
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

EditOP SequenceDecompressor::getSeqCigar (size_t loc, const string &op) {
	size_t  genPos = loc;
	size_t  size = 0;
	char    lastOP = 0;
	int     lastOPSize = 0;
	size_t  endPos = 0;

	string tmpSeq;
	string tmpOP;

	for (size_t pos = 0; pos < op.length(); pos++) {
		if (isdigit(op[pos])) {
			size = size * 10 + (op[pos] - '0');
			continue;
		}
        int t;
		switch (op[pos]) {
			case 'a':
			case 'c':
			case 'g':
			case 't':
			case 'n':
				tmpSeq += toupper(op[pos]);
				genPos++;
				if (lastOP == 'M')
					lastOPSize++;
				else {
					lastOP = 'M';
					lastOPSize = 1;
				}
				break;
			case 'A':
			case 'C':
			case 'G':
			case 'T':
			case 'N': 
				t = 0;
				while (pos < op.length() && (op[pos] == 'A' || op[pos] == 'C' || op[pos] == 'G' || op[pos]=='T' || op[pos] == 'N')) {
					tmpSeq += op[pos];
					pos++;
					t++;
				}
				if (lastOP != 0) {
					tmpOP += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I')
						endPos += lastOPSize;
					lastOP = 0;
					lastOPSize = 0;
				}
				if (op[pos] == '*')
					tmpOP = "*";
				else {
					tmpOP += inttostr(t) + op[pos];
					if (lastOP != 'S' && lastOP != 'I')
						endPos += lastOPSize;
				}
				break;
			case 'D':
			case 'K': // = 'N' in SAM =.= 
				if (lastOP != 0) {
					tmpOP += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I')
						endPos += lastOPSize;
					lastOP = 0;
					lastOPSize = 0;
				}
				tmpOP += inttostr(size) + (op[pos] == 'K' ? 'N' : 'D');
				genPos += size;
				size = 0;
				break;
			case '=':
				if (lastOP == 'M')
					lastOPSize += size;
				else {
					lastOP = 'M';
					lastOPSize = size;
				}
				for (size_t i = 0; i < size; i++) {
					assert(genPos + i < fixedEnd);
					assert(genPos + i >= fixedStart);
					tmpSeq += fixed[genPos + i - fixedStart];
				}
				genPos += size;
				size = 0;
				break;
		}
	}
	if (lastOP != 0) {
		tmpOP += inttostr(lastOPSize) + lastOP;
		if (lastOP != 'S' && lastOP != 'I')
			endPos += lastOPSize;
	}

	EditOP eo;
	eo.start = loc;
	eo.seq = tmpSeq;
	eo.op  = tmpOP;
	eo.end = eo.start + endPos; 

	return eo;
}

