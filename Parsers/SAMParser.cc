#include "../Common.h"
#include "Record.h"
#include "SAMParser.h"

#include <assert.h>
using namespace std;

SAMParser::SAMParser (const string &filename)
{
	Parser::fname = filename;

	if (File::IsWeb(filename)) {
		webFile = WebFile::Download(filename);
		input = (FILE*) webFile->handle();
	} else {
		input = fopen(filename.c_str(), "r");
	}
	if (input == NULL)	
		throw DZException("Cannot open the file %s", filename.c_str());

	fseek(input, 0L, SEEK_END);
	file_size = ftell(input);
	fseek(input, 0L, SEEK_SET);
}

SAMParser::~SAMParser (void) 
{
	fclose(input);
}

string SAMParser::readComment (void)  
{
	string s;
	while (getline(&currentRecord.line, &currentRecord.lineLength, input) != -1) {
		if (currentRecord.line[0] != '@') {
			parse(currentRecord);
			break;
		} else {
			s += currentRecord.line;
		}
	}
	return s;
}

bool SAMParser::readNext ()  
{
	if (getline(&currentRecord.line, &currentRecord.lineLength, input) != -1) {
		assert(currentRecord.line[0] != '@');
		parse(currentRecord);
		return true;
	}
	return false;
}

bool SAMParser::hasNext (void) 
{
	return !feof(input);
}

size_t SAMParser::fpos (void) 
{
	return ftell(input);
}

size_t SAMParser::fsize (void) 
{
	return file_size;
}

void SAMParser::parse (Record &record) 
{
	char *line = &record.line[0];
	int l = strlen(line) - 1;
	while (l && (line[l] == '\r' || line[l] == '\n'))
		line[l--] = 0;
	record.strFields[0] = 0;
	char *c = line;
	int f = 1, sfc = 1, ifc = 0;
	while (*c) {
		if (*c == '\t') {
			if (f == 1 || f == 3 || f == 4 || f == 7 || f == 8)
				record.intFields[ifc++] = atoi(c + 1);
			else
				record.strFields[sfc++] = (c + 1) - line;
			f++;
			*c = 0;
			if (f == 12) break;
		}
		c++;
	}	
	if (f == 11)
		record.strFields[sfc++] = c - line;
	/*while (*c) {
		if (*c == '\t') *c = 0;
		c++;
	}*/
	
	record.intFields[Record::IntField::LOC]--;
	record.intFields[Record::IntField::P_LOC]--;
}

Record SAMParser::next (void) 
{
	return currentRecord;
}

string SAMParser::head (void) 
{
	return currentRecord.getChromosome();
}

template<>
size_t sizeInMemory(Record t) {
    return sizeof(t) + t.getLineLength(); 
}


