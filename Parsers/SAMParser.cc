#include "../Common.h"
#include "Record.h"
#include "SAMParser.h"

#include <assert.h>
using namespace std;

static const int  MAX_LINE_LENGTH = 4096;

SAMParser::SAMParser (const string &filename) {
	input = fopen (filename.c_str(), "r");
	if (input == NULL)	
		throw DZException("Cannot open the file %s", filename.c_str());

	currentLine = new char[MAX_LINE_LENGTH];
	readNext();
}

SAMParser::~SAMParser (void) {
	delete []currentLine;
	fclose(input);
}

bool SAMParser::readNext (void)  {
	while (fgets(currentLine, MAX_LINE_LENGTH, input)) {
		if (currentLine[0] == '@') continue;
		parse();
		return true;
	}
	return false;
}

bool SAMParser::hasNext (void) {
	return !feof(input);
}

void SAMParser::parse (void) {
//	vector<string> fields;
//	int len = strlen(currentLine);
//    if (currentLine[len - 1] == '\n')
//        len--;


	const int MAXFIELD = 1000;
    char qname[MAXFIELD];
    int flag;
    char rname[MAXFIELD];
    int pos;
    char mapq;
    char cigar[MAXFIELD];
    char rnext[MAXFIELD];
    int pnext;
    int tlen;
    char seq[MAXFIELD];
    char qual[MAXFIELD];
    int offset;

    sscanf(currentLine, "%s %d %s %d %d %s %s %d %d %s %s%n",
    		qname, &flag, rname, &pos, &mapq, cigar, rnext, &pnext, &tlen, seq, qual, &offset);
    currentRecord.setQueryName(qname);
	currentRecord.setMappingFlag(flag);
	currentRecord.setMappingReference(rname);
	currentRecord.setMappingLocation(pos);
	currentRecord.setMappingQuality(mapq);
	currentRecord.setMappingOperation(cigar);
	currentRecord.setMateMappingReference(rnext);
	currentRecord.setMateMappingLocation(pnext);
	currentRecord.setTemplateLength(tlen);
	currentRecord.setQuerySeq(seq);
	currentRecord.setQueryQual(qual);
	currentRecord.setOptional(currentLine + offset);

    /*int i;
	for (i = 0; i < len; i++) {
        string tmp = "";
		while (i < len && currentLine[i] != '\t') 
			tmp += currentLine[i++];
		fields.push_back(tmp);
        if (fields.size() == 11)
            break;
	}
    if (i + 1 < len)
        fields.push_back(string(currentLine + i + 1, len - i - 1));
    else 
        fields.push_back("");

	currentRecord.setQueryName(fields[0]);
	currentRecord.setMappingFlag(atoi(fields[1].c_str()));
	currentRecord.setMappingReference(fields[2]);
	currentRecord.setMappingLocation(atoi(fields[3].c_str()));
	currentRecord.setMappingQuality(atoi(fields[4].c_str()));
	currentRecord.setMappingOperation(fields[5]);
	currentRecord.setMateMappingReference((fields[6] == "=")? fields[2]: fields[6]);
	currentRecord.setMateMappingLocation(atoi(fields[7].c_str()));
	currentRecord.setTemplateLength(atoi(fields[8].c_str()));
	currentRecord.setQuerySeq(fields[9]);
	currentRecord.setQueryQual(fields[10]);
    currentRecord.setOptional(fields[11]);*/
}

const Record &SAMParser::next (void) {
	return currentRecord;
}

string SAMParser::head (void) {
	return currentRecord.getMappingReference();
}


