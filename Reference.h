#ifndef Reference_H
#define Reference_H

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdio.h>
#include <string.h>

#include "Common.h"
#include "FileIO.h"

class Stats;

class Reference {
	friend class Stats;

public:
	struct Chromosome {
		std::string chr;
		std::string md5;
		std::string filename;
		size_t len;
		size_t loc;
	};

private:
	shared_ptr<File> input;
	std::string directory, filename;
	std::string currentChr;

	std::unordered_map<std::string, Chromosome> 
		chromosomes;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
		samCommentData;
	std::string currentWebFile;

private:
	std::string buffer;
	size_t bufferStart, bufferEnd, currentPos;
	char c;
	
public:
	Reference (const std::string &filename);
	~Reference (void);

public:
	std::string getChromosomeName(void) const;
	std::string scanChromosome(const std::string &s);
	//void load(char *arr, size_t s, size_t e);
	size_t getChromosomeLength(const std::string &s) const;
	void scanSAMComment (const std::string &comment);

	Chromosome getChromosomeInfo (const std::string &chr) {
		return chromosomes[chr];
	}

	void loadIntoBuffer(size_t end);
	char operator[](size_t pos) const;
	std::string copy(size_t start, size_t end);
	void trim(size_t from);

private:
	void loadFromFASTA (const std::string &fn);
};

#endif
