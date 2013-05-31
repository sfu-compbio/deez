#include "../Common.h"
#include "Record.h"
#include "SAMParser.h"
using namespace std;

static const char *LOG_PREFIX     = "<Parser>";
static const int  MAX_LINE_LENGTH = 4096;

SAMParser::SAMParser (const string &filename) {
	input = fopen (filename.c_str(), "r");
	if (input == NULL)	
		throw DZException("Cannot open the file %s", filename.c_str());

	currentLine = new char[MAX_LINE_LENGTH];
	of.resize(256 * 256, 0);
	readNext();
}

SAMParser::~SAMParser (void) {
	delete []currentLine;
	fclose(input);
}

bool SAMParser::readNext (void)  {
	if (fgets(currentLine, MAX_LINE_LENGTH, input)) {
		parse();
		return true;
	}
	return false;
}

bool SAMParser::hasNext (void) {
	return !feof(input);
}

void SAMParser::parse (void) {
	vector<string> fields;
	int len = strlen(currentLine);
    if (currentLine[len - 1] == '\n') 
        len--;
	for (int i = 0; i < len; i++) {
		string tmp;
		tmp.reserve(50);
		while (i < len && currentLine[i] != '\t') {
			tmp += currentLine[i];
			i++;
		}
		fields.push_back(tmp);
	}

	int k = 11;
	string keys = "";
	string values = "";
	keys.reserve((fields.size() - 11)*3);
	values.reserve(50);
	char a,b,c;
	string kk = "";
	while (k < fields.size()) {
		a = fields[k][0];
		b = fields[k][1];
		c = fields[k][3];

		if (of[a*b] == 0) {
			kk = a;
			kk += b;
			kk += c;
			ofm.push_back(fields[k].substr(0,5));
			of[a * b] = ofm.size();
			keys += (unsigned char)ofm.size();
		}
		else
		{
			keys+=(unsigned char)of[a*b];
		}
		if (values != "")
			values += '|';
		values += fields[k].substr(5);
		k++;
	}


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
	currentRecord.setOptionalFieldKey(keys);
	currentRecord.setOptionalFieldValue(values);
}

const Record &SAMParser::next (void) {
	return currentRecord;
}

string SAMParser::head (void) {
	return currentRecord.getMappingReference();
}

string SAMParser::getOFMeta (void) {
	string tmp="";
	for (int i=0; i<ofm.size(); i++) {
		tmp+=ofm[i];
		//logger->log("%s", ofm[i].c_str());
	}
	LOG("%s", tmp.c_str());
	return tmp;
}

