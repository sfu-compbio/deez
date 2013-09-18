#include "../Common.h"
#include "Record.h"
#include "SAMParser.h"

#include <assert.h>
using namespace std;

SAMParser::SAMParser (const string &filename) {
	input = fopen (filename.c_str(), "r");
	if (input == NULL)	
		throw DZException("Cannot open the file %s", filename.c_str());

	fseek(input, 0L, SEEK_END);
	file_size = ftell(input);
	fseek(input, 0L, SEEK_SET);
}

SAMParser::~SAMParser (void) {
	fclose(input);
}

char *SAMParser::readComment (void)  {
	fgets(currentRecord.line, MAXLEN, input);
	if (currentRecord.line[0] != '@') {
		parse();
		return 0;
	}
	return currentRecord.line;
}

bool SAMParser::readNext (void)  {
	while (fgets(currentRecord.line, MAXLEN, input)) {
		assert(currentRecord.line[0] != '@');
		parse();
		return true;
	}
	return false;
}

bool SAMParser::hasNext (void) {
	return !feof(input);
}

size_t SAMParser::fpos (void) {
	return ftell(input);
}

size_t SAMParser::fsize (void) {
	return file_size;
}

void SAMParser::parse (void) {
	int l = strlen(currentRecord.line) - 1;
	while (l && (currentRecord.line[l] == '\r' || currentRecord.line[l] == '\n'))
		currentRecord.line[l--] = 0;
	char *c = currentRecord.fields[0] = currentRecord.line;
	int f = 1;
	while (*c) {
		if (*c == '\t') {
			currentRecord.fields[f++] = c + 1;
			*c = 0;
			if (f == 12) break;
		}
		c++;
	}	
	if (f == 11)
		currentRecord.fields[11] = c;
}

const Record &SAMParser::next (void) {
	return currentRecord;
}

string SAMParser::head (void) {
	return currentRecord.getChromosome();
}


