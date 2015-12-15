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
	FILE *input;
	shared_ptr<File> webFile;
	
	Record currentRecord;
    size_t file_size;

public:
	SAMParser (const std::string &filename);
	~SAMParser (void);

public:
	std::string readComment (void);
	bool readNext (void);
	bool readNextTo (Record &record);
	//bool readRaw(Array<uint8_t> &a);
	bool hasNext (void);
	size_t fpos (void);
	size_t fsize (void);

public:
	void parse (Record &line);
	const Record &next (void);
	std::string head (void);
};

#endif
