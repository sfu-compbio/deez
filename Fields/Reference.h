#ifndef Reference_H
#define Reference_H

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "../Common.h"
#include "../Engines/Engine.h"
#include "EditOperation.h"

class Reference {
	FILE *input;
	std::string chromosomeName;
	std::string chromosome;

public:
	Reference (const std::string &filename);
	~Reference (void);

public:
	std::string getChromosomeName (void) const;
	size_t getChromosomeLength (void) const;
	size_t readNextChromosome (void);

public:
	char operator[](size_t i) const;

private:
	std::vector<std::string> chrStr;
	std::map<std::string,int> chrInt;

public:
	std::string getChromosomeIndex (int i) {
		if (i < 0 || i > (int)chrStr.size())
			return "*";
		return chrStr[i];
	}

	int getChromosomeIndex (const std::string &i) {
		if (i == "*")
			return -1;
		return chrInt[i];
	}
};		

struct EditOP {
	int start;
	int end;
	std::string seq;
	std::string op;
};

struct GenomeChanges {
	uint32_t	loc;
	uint8_t 	original;
	uint8_t	changed;
};

class ReferenceCompressor: public Compressor {
	gzFile file;

	Reference reference;
	EditOperationCompressor editOperation;

	std::string 			  	fixedGenome; 	// genome
	std::vector<int*> 	        doc;     		// stats
	std::vector<GenomeChanges>  fixes;   		// fixes

public:
	ReferenceCompressor (const string &filename, const std::string &refFile, int bs);
	~ReferenceCompressor (void);

public:
	std::vector<EditOP> records;

	void addRecord (EditOP &eo) {
		eo.end = eo.start + updateGenome(eo.start - 1, eo.seq, eo.op);
		records.push_back(eo);
	}

	int getBlockBoundary (int lastFixedLocation) {
		std::vector<EditOP>::iterator it;
		int k = 0;
		for (it = records.begin(); it < records.end() && it->end < lastFixedLocation; it++) {
			k++;
			editOperation.addRecord(getEditOP(it->start - 1, it->seq, it->op));
		}
		records.erase(records.begin(), records.begin() + k);
		return k;
	}

	void outputRecords (vector<char> &output) {
		editOperation.outputRecords(output);
	}

	int getChromosome (const std::string &i) {
		return reference.getChromosomeIndex(i);
	}

private:
	void outputChanges (void);

public:
	std::string getName (void);
	size_t getLength (void);
	bool getNext (void);

private:
	inline char getDNAValue (char ch);
	inline void updateGenomeLoc (int loc, char ch);

public:
	int updateGenome (int loc, const std::string &seq, const std::string &op);
	void fixGenome (int start, int end);
	std::string getEditOP (int loc, const std::string &seq, const std::string &op);
};

class ReferenceDecompressor: public Decompressor {
	gzFile 	  file;
	Reference reference;
	EditOperationDecompressor editOperation;

	std::string fixedName;
	std::string fixedGenome; 
	std::vector<GenomeChanges> fixes;

public:
	ReferenceDecompressor (const std::string &filename, const std::string &refFile, int bs);
	~ReferenceDecompressor (void);

private:
	void getChanges (void);

public:
	std::string getName (void);
	bool getNext (void);

public:
	std::string getChromosome (int i) {
		return reference.getChromosomeIndex(i);
	}

private:
	EditOP getSeqCigar (int loc, const std::string &op);

public:
	void importRecords (const std::vector<char> &input) {
		editOperation.importRecords(input);
	}

	bool hasRecord (void) {
		return editOperation.hasRecord();
	}

	EditOP getRecord (int loc) {
		return getSeqCigar(loc, editOperation.getRecord());
	}
};

#endif
