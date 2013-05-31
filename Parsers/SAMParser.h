#ifndef SAMParser_H
#define SAMParser_H

#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>

#include "../Common.h"
#include "Parser.h"
#include "Record.h"

class SAMParser: public Parser {
	Record currentRecord;
    char   *currentLine;

	std::vector<std::string> ofm;
	std::vector<short> of;

public:
	SAMParser (const std::string &filename);
	~SAMParser (void);

public:
	bool readNext (void);
	bool hasNext (void);

public:
	void parse (void);
	const Record &next (void);
	std::string head (void);
	std::string getOFMeta (void);
};

#endif
