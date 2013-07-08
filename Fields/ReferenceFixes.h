#ifndef ReferenceFixes_H
#define ReferenceFixes_H

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "../Common.h"
#include "../Reference.h"
#include "../Engines/Engine.h"
#include "EditOperation.h"

struct EditOP {
	size_t start;
	size_t end;
	std::string seq;
	std::string op;
};

struct GenomeChanges {
	uint32_t loc;
	uint8_t original;
	uint8_t	changed;
};

class ReferenceFixesCompressor: public Compressor {
	gzFile file;

	Reference reference;
	EditOperationCompressor editOperation;

	std::string 			  	fixedGenome; 	// genome
	std::vector<int*> 	        doc;     		// stats
	std::vector<GenomeChanges>  fixes;   		// fixes

	FILE *__tf__;

public:
	ReferenceFixesCompressor (const string &filename, const std::string &refFile, int bs);
	~ReferenceFixesCompressor (void);

public:
	std::vector<EditOP> records;

	void addRecord (EditOP &eo) {
		eo.end = eo.start + updateGenome(eo.start - 1, eo.seq, eo.op);
		records.push_back(eo);
	}

	size_t getBlockBoundary (size_t lastFixedLocation) {
		std::vector<EditOP>::iterator it;
		size_t k = 0;
		string s;
		for (it = records.begin(); it < records.end() && it->end < lastFixedLocation; it++) {
			k++;
			editOperation.addRecord(s = getEditOP(it->start - 1, it->seq, it->op));
			fwrite(s.c_str(), 1, s.size()+1, __tf__);
		}
		LOG("k=%lu of %'lu, lfo=%'lu,f1=%'lu,fE=%'lu",k,records.size(),
			lastFixedLocation, records[0].end, records[records.size()-1].end );
		records.erase(records.begin(), records.begin() + k);
		return k;
	}

	uint64_t perchr;
	void outputRecords (vector<char> &output) {
		editOperation.outputRecords(output);
		perchr += output.size();
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
	inline void updateGenomeLoc (size_t loc, char ch);

public:
	size_t updateGenome (size_t loc, const std::string &seq, const std::string &op);
	void fixGenome (size_t start, size_t end);
	std::string getEditOP (size_t loc, const std::string &seq, const std::string &op);
};

class ReferenceFixesDecompressor: public Decompressor {
	gzFile 	  file;
	Reference reference;
	EditOperationDecompressor editOperation;

	std::string fixedName;
	std::string fixedGenome; 
	std::vector<GenomeChanges> fixes;

public:
	ReferenceFixesDecompressor (const std::string &filename, const std::string &refFile, int bs);
	~ReferenceFixesDecompressor (void);

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
	EditOP getSeqCigar (size_t loc, const std::string &op);

public:
	void importRecords (const std::vector<char> &input) {
		editOperation.importRecords(input);
	}

	bool hasRecord (void) {
		return editOperation.hasRecord();
	}

	EditOP getRecord (size_t loc) {
		return getSeqCigar(loc, editOperation.getRecord());
	}
};

#endif
