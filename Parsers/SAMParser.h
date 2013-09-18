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
    size_t file_size;

public:
	SAMParser (const std::string &filename);
	~SAMParser (void);

public:
	char *readComment (void);
	bool readNext (void);
	bool hasNext (void);
	size_t fpos (void);
	size_t fsize (void);

public:
	void parse (void);
	const Record &next (void);
	std::string head (void);
};

#endif
