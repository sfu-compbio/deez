#include "ReferenceFixes.h"
using namespace std;

inline string inttostr (int k) {
    static vector<std::string> mem;
    if (mem.size() == 0) {
        mem.reserve(10000);
        mem.push_back("0");
        for (int i = 1; i < 10000; i++) {
            string s = "";
            int p = i;
            while (p) {
                s = char(p % 10 + '0') + s;
                p /= 10;
            }
            mem.push_back(s);
        }
    }
    return mem[k];
}

inline char getDNAValue (char ch) {
	switch (toupper(ch)) {
		case '.':
			return 0;
		case 'A':
		case 'a':
			return 1;
		case 'C':
		case 'c':
			return 2;
		case 'G':
		case 'g':
			return 3;
		case 'T':
		case 't':
			return 4;
		default:
			return 5;
	}
}

/********************************************************************************/
/*
Reference.chromosome ...... // current chromosome
SweepNext ~~~
Print block.
*/

ReferenceFixesCompressor::ReferenceFixesCompressor (const string &refFile, int bs):
	reference(refFile), 
	editOperation(bs),
	fixes(bs),
	genomePager(0),
	chromosome("") // first call to scanNextChr will populate this one
{
}

ReferenceFixesCompressor::~ReferenceFixesCompressor (void) {
}


void ReferenceFixesCompressor::addRecord (size_t loc, const string &seq, const string &cigar) {
	EditOP eo;
	eo.end = loc + updateGenome(eo.start = loc, eo.seq = seq, eo.op = cigar);
	records.push_back(eo);
}

void ReferenceFixesCompressor::outputRecords (Array<uint8_t> &output) {
	output.resize(0);
	size_t sz = 0;
	output.add((uint8_t*)&sz, sizeof(size_t));

	// fixes
	if (chromosome != "*") {
		fixes.appendRecords(output);
		output.add((uint8_t*)&blockStart, sizeof(size_t));
		output.add((uint8_t*)&lastFixed, sizeof(size_t));
		output.add((uint8_t*)&nextStart, sizeof(size_t));
		sz = output.size() - sizeof(size_t);
		*(size_t*)output.data() = sz;
	}

	// operations
	editOperation.appendRecords(output);
}

void ReferenceFixesCompressor::scanNextChromosome (void) {
	// by here, all should be fixed ...
	assert(fixes.size() == 0);
	assert(records.size() == 0);
	assert(editOperation.size() == 0);

	// clean genomePager
	while (genomePager) {
		GenomePager *gp = genomePager->next;
		delete genomePager;
		genomePager = gp;
	}
	genomePager = new GenomePager(0); // new one
	lastFixed = blockStart = 0;

	chromosome = reference.scanNextChromosome();
}

// called at the end of the block!
// what if end is chr_end?
void ReferenceFixesCompressor::applyFixes (size_t end) {
	/////////////////////////////////////////////////////////////////////////////
	size_t k;

	if (chromosome != "*") {
	//	DEBUG("Sent %'lu", end);
		// update blockEnd to handle cigar skips
		size_t blockEnd = end;
		blockStart = lastFixed;
		
		assert(blockStart <= blockEnd);
		DEBUG("Applying fixes %'lu-%'lu", blockStart, blockEnd);

		// update pages
		GenomePager *gp = genomePager;
		assert(gp != NULL);
		// start should be in the first page
		assert(blockStart >= gp->start);
		assert(blockStart < gp->start + GenomePager::GenomePageSize);
		size_t s, e;
		for (k = blockStart; k < blockEnd && gp; ) {
			s = max(gp->start, k) - gp->start;
			e = min(gp->start + GenomePager::GenomePageSize, blockEnd) - gp->start;

			end = gp->start + e;

			reference.load(gp->fixes + s, gp->start + s, gp->start + e);
			// reference index has to be accesses in non-decreasing order through the application lifetime!
			for (size_t i = s; i < e; i++) 
				if (gp->stat[i]) {
					int max = -1, pos = -1;
					for (int j = 1; j < 6; j++)
						if (gp->stat[i][j] > max)
							max = gp->stat[i][pos = j];
					if (gp->fixes[i] != pos[".ACGTN"]) 
						fixes.addRecord(GenomeChanges(gp->start + i, gp->fixes[i] = pos[".ACGTN"]));
					delete[] gp->stat[i];
					gp->stat[i] = 0;
				}
			gp = gp->next;
			k += GenomePager::GenomePageSize;
			k -= k % GenomePager::GenomePageSize;
		}
		blockEnd = end;
	//	DEBUG("Finally fixed to %'lu", blockEnd);
	}
	//else DEBUG("Fixing *");

	lastFixed = end;

	// now fix cigars in the database
	for (k = 0; k < records.size() && records[k].end < end; k++)
		editOperation.addRecord(getEditOP(records[k].start, records[k].seq, records[k].op));
	if (k < records.size())
		nextStart = records[k].start;
	else
		nextStart = (size_t)-1;
	records.erase(records.begin(), records.begin() + k);

	if (chromosome != "*") {
		// all till the end is fixed
		GenomePager *gp = genomePager;
		while (gp && gp->start + GenomePager::GenomePageSize < end) {
		//	DEBUG("GP %'lu %'lu",gp->start,gp->start+GenomePager::GenomePageSize);
		//	string x = string(gp->fixes, GenomePager::GenomePageSize);
		//	DEBUG(">>> %'lu ~ %-lu", gp->start, gp->start + GenomePager::GenomePageSize);
		//	DEBUG(">>> %s",x.c_str());
			genomePager = gp->next; delete gp; gp = genomePager;
		}
		if (!genomePager) genomePager = new GenomePager(0);
	}
	// return k;
}

inline void ReferenceFixesCompressor::updateGenomeLoc (size_t loc, char ch) {
	genomePager->increase(loc, getDNAValue(ch));
}

size_t ReferenceFixesCompressor::updateGenome (size_t loc, const string &seq, const string &op) {
	if (op == "*") 
		return 0;

	size_t size = 0;
	size_t genPos = loc;
	size_t seqPos = 0;
	size_t spanSize = 0;

	for (size_t pos = 0; pos < op.length(); pos++) {
		if (isdigit(op[pos])) {
			size = size * 10 + (op[pos] - '0');
			continue;
		}
		switch (op[pos]) {
			case 'M':
			case '=':
			case 'X':
				for (size_t i = 0; i < size; i++) {
					updateGenomeLoc(genPos, seq[seqPos]);
					genPos++;
					seqPos++;
				}
				spanSize += size;
				break;
			case 'I':
			case 'S':
				seqPos += size;
				break;
			case 'D':
			case 'N':
				for (size_t i = 0; i < size; i++) {
					updateGenomeLoc(genPos, '.');
					genPos++;
				}
				spanSize += size;
				break;
		}
		size = 0;
	}
	return spanSize;
}

string ReferenceFixesCompressor::getEditOP(size_t loc, const string &seq, const string &op) {
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
					if (seq[seqPos] == genomePager->getFixed(genPos)) {
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

	return newOP; //res;
}

/************************************************************************************************************************/

ReferenceFixesDecompressor::ReferenceFixesDecompressor (const string &refFile, int bs):
	reference(refFile), 
	editOperation(bs),
	fixes(bs),
	chromosome(""),
	fixed(0),
	fixed_size(0),
	nextStart((size_t)-1)
{
}

ReferenceFixesDecompressor::~ReferenceFixesDecompressor (void) {
	delete[] fixed;
}


bool ReferenceFixesDecompressor::hasRecord (void) {
	return editOperation.hasRecord();
}

EditOP ReferenceFixesDecompressor::getRecord (size_t loc) {
	return getSeqCigar(loc, editOperation.getRecord());
}

void ReferenceFixesDecompressor::importRecords (uint8_t *in, size_t in_size) {
	size_t fixes_sz = *(size_t*)in;
	importFixes(in + sizeof(size_t), fixes_sz);
	editOperation.importRecords(in + fixes_sz + sizeof(size_t), in_size - fixes_sz - sizeof(size_t));
}

void ReferenceFixesDecompressor::importFixes (uint8_t *in, size_t in_size) {
	if (chromosome == "*")
		return;
	if (in_size == 0) 
		return;
	fixes.importRecords(in, in_size - 3 * sizeof(size_t));
	size_t *bnd = (size_t*)(in + in_size - 3 * sizeof(size_t));

	if (nextStart == (size_t)-1) 
		nextStart = bnd[0];

	if (nextStart > bnd[0])
		nextStart = bnd[0];
	char *c = new char[bnd[1] - nextStart];
	if (nextStart < bnd[0]) {
		assert(fixed_size - bnd[0] + nextStart >= 0);
		assert(fixed_size - bnd[0] + nextStart < fixed_size);
		memcpy(c, fixed + (fixed_size - bnd[0] + nextStart), bnd[0] - nextStart);
	}
	delete[] fixed;
	fixed = c;
	fixed_size = bnd[1] - nextStart;
	fixed_offset = nextStart;
	reference.load(fixed + bnd[0] - nextStart, bnd[0], bnd[1]);

	while (fixes.hasRecord()) {
		GenomeChanges gc = fixes.getRecord();
		assert(gc.loc - fixed_offset < fixed_size);
		fixed[gc.loc - fixed_offset] = gc.changed;
	}

	nextStart = bnd[2];
}

void ReferenceFixesDecompressor::scanNextChromosome (void) {
	// by here, all should be fixed ...
// 	TODO more checking
//	assert(fixes.size() == 0);
//	assert(records.size() == 0);
//	assert(editOperation.size() == 0);

	// clean genomePager
	chromosome = reference.scanNextChromosome();
}

EditOP ReferenceFixesDecompressor::getSeqCigar (size_t loc, const string &op) {
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
				tmpOP += inttostr(size) + op[pos];
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
					assert((int)genPos + (int)i - (int)fixed_offset >= 0);
					assert(genPos + i - fixed_offset < fixed_size);
					tmpSeq += fixed[genPos + i - fixed_offset];
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

