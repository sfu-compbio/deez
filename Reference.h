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
	std::string currentChr;
	size_t currentPos;
	char c;
	
public:
	Reference (const std::string &filename);
	~Reference (void);

public:
	std::string getChromosomeName (void) const;
	std::string scanNextChromosome (void);
	void load (char *arr, size_t s, size_t e);
};

#endif
