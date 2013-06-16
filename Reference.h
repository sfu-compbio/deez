#ifndef Reference_H
#define Reference_H

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>

#include "Common.h"

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

#endif
