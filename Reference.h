#ifndef Reference_H
#define Reference_H

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdio.h>
#include <string.h>

#ifdef OPENSSL
#include <openssl/md5.h>
#endif

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
	File *input;
	std::string directory, filename;
	std::string currentChr;
	size_t currentPos;
	char c;

	std::unordered_map<std::string, Chromosome> 
		chromosomes;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
		samCommentData;
	std::string currentWebFile;
	
public:
	Reference (const std::string &filename);
	~Reference (void);

public:
	std::string getChromosomeName(void) const;
	std::string scanChromosome(const std::string &s);
	void load(char *arr, size_t s, size_t e);
	size_t getChromosomeLength(const std::string &s) const;
	void scanSAMComment (const std::string &comment);

	Chromosome getChromosomeInfo (const std::string &chr) {
		return chromosomes[chr];
	}

private:
	void loadFromFASTA (const std::string &fn);

};

#endif
