#include "Reference.h"
using namespace std;

static const char *LOG_PREFIX    = "<Ref>";
static const int  MAX_CHROMOSOME = 400 * MB;

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

Reference::Reference (const string &filename) {
	input = fopen(filename.c_str(), "rb");
	if (input == NULL) 
		throw DZException("Cannot open the file %s", filename.c_str());
	chromosome.reserve(MAX_CHROMOSOME);

    char chrName[100];
    int chrIdx = 0;
    while (fgets(chrName, 100, input))
        if (chrName[0] == '>') {
            string cn = string(chrName + 1);
            cn.pop_back();
            chrStr.push_back(cn);
            chrInt[cn] = chrIdx++;
        }
    chrStr.push_back("*");
    chrInt["*"] = chrIdx;
    fseek(input, 0, SEEK_SET);
}

Reference::~Reference (void) {
	fclose(input);
}

string Reference::getChromosomeName (void) const {
	return chromosomeName;
}

size_t Reference::getChromosomeLength (void) const {
	return chromosome.size();
}

size_t Reference::readNextChromosome (void) {
	if (feof(input)) {
		chromosomeName = "*";
		return 0;
	}
    char c;
//	if (lastChar == '>') {
	chromosomeName = "";
	c = fgetc(input); // avoid >
    c = fgetc(input);
	while (c != '\r' && c != '\n' && c != EOF) {
		chromosomeName += c;
		c = fgetc(input);
	}
	while (!feof(input)) {
		if (c == '>')
			break;
		if (c != '\n' && c != '\r')
			chromosome += toupper(c);
		c = fgetc(input);
	}
	LOG("%s (%d) loaded", chromosomeName.c_str(), chromosome.size());

	return chromosome.size();
}

char Reference::operator[](size_t i) const {
    if (i > chromosome.size())
        throw DZException("Access outside chromosome range");
    return chromosome[i];
}

ReferenceCompressor::ReferenceCompressor (const string &filename, const string &refFile, int bs): 
	reference(refFile), editOperation(filename, bs) {
	string name1(filename + ".rf.dz");
	file = gzopen(name1.c_str(), "wb6");
	if (file == Z_NULL) 
		throw DZException("Cannot open the file %s", name1.c_str());
}

ReferenceCompressor::~ReferenceCompressor (void) {
	outputChanges();
	gzclose(file);
}

void ReferenceCompressor::outputChanges (void) {
	if (fixes.size() > 0) {
        gzwrite(file, reference.getChromosomeName().c_str(), reference.getChromosomeName().length() + 1);
		int size = fixes.size();
		gzwrite(file, &size, sizeof(int));
		gzwrite(file, &fixes[0], fixes.size() * sizeof(GenomeChanges));
		fixes.clear();
		LOG("Genome Changes Written");
	}
}

string ReferenceCompressor::getName (void) {
	return reference.getChromosomeName();
}

size_t ReferenceCompressor::getLength (void) {
	return reference.getChromosomeLength();
}

bool ReferenceCompressor::getNext (void) {
	outputChanges();

	LOG("Loading Reference Genome ...");
	size_t cnt = reference.readNextChromosome();

	fixedGenome.resize(cnt);
	doc.resize(cnt, 0);

	return cnt;
}

inline char ReferenceCompressor::getDNAValue (char ch) {
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

inline void ReferenceCompressor::updateGenomeLoc (int loc, char ch) {
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

int ReferenceCompressor::updateGenome (int loc, const string &seq, const string &op) {
	if (op == "*") 
		return 0;

	int size = 0;
	int genPos = loc;
	int seqPos = 0;
	int spanSize = 0;

	for (int pos = 0; pos < op.length(); pos++) {
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

void ReferenceCompressor::fixGenome(int start, int end) {
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

string ReferenceCompressor::getEditOP(int loc, const string &seq, const string &op) {
	if (op == "*") {
		return seq + "*";
	}

	string newOP;

	int size   = 0;
	int genPos = loc;
	int seqPos = 0;

	char lastOP = 0;
	int  lastOPSize = 0;

	for (int pos = 0; pos < op.size(); pos++) {
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
					else if(lastOP == 'X') 
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
	return newOP;
}

ReferenceDecompressor::ReferenceDecompressor (const string &filename, const string &refFile, int bs)
    : reference(refFile), editOperation(filename, bs) {
	string name1(filename + ".rf.dz");
	file = gzopen(name1.c_str(), "rb");
	if (file == Z_NULL) 
		throw DZException("Cannot open the file %s", name1.c_str());

	getChanges();
}

ReferenceDecompressor::~ReferenceDecompressor (void) {
	gzclose(file);
}

void ReferenceDecompressor::getChanges (void) {
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

string ReferenceDecompressor::getName (void) {
	return reference.getChromosomeName();
}

bool ReferenceDecompressor::getNext (void) {
	LOG("Loading Reference Genome ...");
	size_t cnt = reference.readNextChromosome();
	fixedGenome.resize(cnt);

	for (size_t i = 0; i < cnt; i++)
		fixedGenome[i] = reference[i];
	if (fixedName == reference.getChromosomeName()) {
		vector<GenomeChanges>::iterator it;
		for (int i = 0; i < fixes.size(); i++)
			fixedGenome[fixes[i].loc] = fixes[i].changed;
	}
	fixes.clear();
	getChanges();

	return (cnt > 0);
}

EditOP ReferenceDecompressor::getSeqCigar (int loc, const string &op) {
	int  genPos  	 = loc;
	int  size    	 = 0;
	char lastOP 	 = 0;
	int  lastOPSize = 0;
	int  endPos		 = 0;

	string tmpSeq;
	string tmpOP;

	for (int pos = 0; pos < op.length(); pos++) {
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

