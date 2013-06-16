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

ReferenceFixesCompressor::ReferenceFixesCompressor (const string &filename, const string &refFile, int bs):
	reference(refFile), editOperation(bs) 
{
	stream = new GzipCompressionStream<6>();

	string name1(filename + ".rf.dz");
	file = gzopen(name1.c_str(), "wb6");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	perchr=0;
}

ReferenceFixesCompressor::~ReferenceFixesCompressor (void) {
	outputChanges();
	gzclose(file);
	delete stream;
}

void ReferenceFixesCompressor::outputChanges (void) {
	if (fixes.size() > 0) {
        gzwrite(file, reference.getChromosomeName().c_str(), reference.getChromosomeName().length() + 1);
		int size = fixes.size();
		gzwrite(file, &size, sizeof(int));
		gzwrite(file, &fixes[0], fixes.size() * sizeof(GenomeChanges));
		fixes.clear();
		LOG("Genome Changes Written");
		LOG("For this %'llu EOps", perchr); perchr=0;
	}
}

string ReferenceFixesCompressor::getName (void) {
	return reference.getChromosomeName();
}

size_t ReferenceFixesCompressor::getLength (void) {
	return reference.getChromosomeLength();
}

bool ReferenceFixesCompressor::getNext (void) {
	outputChanges();

	LOG("Loading Reference Genome ...");
	size_t cnt = reference.readNextChromosome();

	fixedGenome.resize(cnt);
	doc.resize(cnt, 0);

	return (bool)cnt;
}

inline char ReferenceFixesCompressor::getDNAValue (char ch) {
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

inline void ReferenceFixesCompressor::updateGenomeLoc (int loc, char ch) {
	int pos = getDNAValue(ch);
	if (doc[loc])
		doc[loc][pos]++;
	else {
		int *tmp = new int[6];
		memset(tmp, 0, 6 * sizeof(int)); 
		tmp[pos] = 1;
		doc[loc] = tmp;
	}
}

int ReferenceFixesCompressor::updateGenome (int loc, const string &seq, const string &op) {
	if (op == "*") 
		return 0;

	int size = 0;
	int genPos = loc;
	int seqPos = 0;
	int spanSize = 0;

	for (size_t pos = 0; pos < op.length(); pos++) {
		if (isdigit(op[pos])) {
			size = size * 10 + (op[pos] - '0');
			continue;
		}
		switch (op[pos]) {
			case 'M':
			case '=':
			case 'X':
				for (int i = 0; i < size; i++) {
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
				for (int i = 0; i < size; i++) {
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

void ReferenceFixesCompressor::fixGenome (int start, int end) {
	LOG("Updating %s:%d-%d ...", reference.getChromosomeName().c_str(), start, end);
	for (int i = start; i < end; i++) {
		if (doc[i] == 0)
			fixedGenome[i] = reference[i];
		else {
			int *tmp = doc[i];
			int max = -1;
			int pos = -1;
			for (int j = 1; j < 6; j++)
				if (tmp[j] > max) {
					max = tmp[j];
					pos = j;
				}
			doc[i] = 0;

			int a[] = {'.', 'A', 'C', 'G', 'T', 'N'};

			fixedGenome[i] = a[pos];
			if (reference[i] != a[pos]) {
				GenomeChanges f;
				f.loc = i;
				f.original = reference[i];
				f.changed = fixedGenome[i];
				fixes.push_back(f);
			}
			delete[] tmp;
		}
	}
    LOG("%s:%d-%d is updated.", reference.getChromosomeName().c_str(), start, end);
}

//CigarOp
string ReferenceFixesCompressor::getEditOP(int loc, const string &seq, const string &op) {
//	CigarOp res;

	if (op == "*") {
//		res.keys = seq + "*";
//		return res;
		return seq+"*";
	}

	string newOP;

	int size   = 0;
	int genPos = loc;
	int seqPos = 0;

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
				for (int i = 0; i < size; i++) {
					if (seq[seqPos] == fixedGenome[genPos]) {
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
				for (int i = 0; i < size; i++)
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

ReferenceFixesDecompressor::ReferenceFixesDecompressor (const string &filename, const string &refFile, int bs):
	reference(refFile), 
	editOperation(bs) 
{
	string name1(filename + ".rf.dz");
	file = gzopen(name1.c_str(), "rb");
	if (file == Z_NULL) 
		throw DZException("Cannot open the file %s", name1.c_str());

	getChanges();
}

ReferenceFixesDecompressor::~ReferenceFixesDecompressor (void) {
	gzclose(file);
}

void ReferenceFixesDecompressor::getChanges (void) {
	int size = 0;
	char ch;
	if (gzread(file, &ch, 1)) {
		fixedName = "";
		while (ch) {
			fixedName += ch;
			gzread(file, &ch, 1);
		}
		gzread(file, &size, sizeof(int));
		fixes.resize(size);
		gzread(file, &fixes[0], size * sizeof(GenomeChanges));
	}
	LOG("Genome Changes Loaded");
}

string ReferenceFixesDecompressor::getName (void) {
	return reference.getChromosomeName();
}

bool ReferenceFixesDecompressor::getNext (void) {
	LOG("Loading Reference Genome ...");
	size_t cnt = reference.readNextChromosome();
	fixedGenome.resize(cnt);

	for (size_t i = 0; i < cnt; i++)
		fixedGenome[i] = reference[i];
	if (fixedName == reference.getChromosomeName()) {
		vector<GenomeChanges>::iterator it;
		for (size_t i = 0; i < fixes.size(); i++)
			fixedGenome[fixes[i].loc] = fixes[i].changed;
	}
	fixes.clear();
	getChanges();

	return (cnt > 0);
}

EditOP ReferenceFixesDecompressor::getSeqCigar (int loc, const string &op) {
	int  genPos  	 = loc;
	int  size    	 = 0;
	char lastOP 	 = 0;
	int  lastOPSize = 0;
	int  endPos		 = 0;

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
				for (int i = 0; i < size; i++)
					tmpSeq += fixedGenome[genPos + i];
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

