#include "OptionalField.h"
using namespace std;

static const char *LOG_PREFIX = "<OF>";
static const int  RECORD_AVG 	= 100;

OptionalFieldCompressor::OptionalFieldCompressor (const string &filename, int blockSize) {
	string name1(filename + ".of.dz");
	file = gzopen(name1.c_str(), "wb6");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
	
    fieldIdx.resize(256 * 256, 0);
    fieldData.resize(256 * 256);
    fieldCount = 0;
}

OptionalFieldCompressor::~OptionalFieldCompressor (void) {
	gzclose(file);
}

/*
All optional elds follow the TAG:TYPE:VALUE format where TAG is a two-character string that matches
/[A-Za-z][A-Za-z0-9]/. Each TAG can only appear once in one alignment line. A TAG containing
lowercase letters are reserved for end users. In an optional field, TYPE is a single casesensitive letter
which denes the format of VALUE:
A [!-~] Printable character
i [-+]?[0-9]+ Singed 32-bit integer
f [-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)? Single-precision oating number
Z [ !-~]+ Printable string, including space
H [0-9A-F]+ Byte array in the Hex format1
B [cCsSiIf](,[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)+ Integer or numeric array

KEY multiType!
*/
void OptionalFieldCompressor::addRecord (const string &rec) {
	vector<int> keys;
    for (int i = 0; i < rec.size(); i++) {
        string s;
        int j = i;
        while (j < rec.size() && rec[j] != '\t') j++;
        s = rec.substr(i, j - i);
        if (s.size() < 5)
            throw DZException("SAM optional tag is invalid!");
        short idx = rec[0] * 256 + rec[1];
        if (!fieldIdx[idx]) 
            fieldIdx[idx] = ++fieldCount;
        keys.push_back(fieldIdx[idx]);
        fieldData[idx].push_back(rec.substr(5));
        i = j;
    }
}

void OptionalFieldCompressor::outputRecords (void) {
	if (buffer.size() > 0)
		gzwrite(file, buffer.c_str(), buffer.size());
	buffer.clear();
}

OptionalFieldDecompressor::OptionalFieldDecompressor (const string &filename, int bs)
	: blockSize (bs), recordCount (0) {
	string name1(filename + ".of.dz");
	file = gzopen(name1.c_str(), "rb");
	if (file == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
}

OptionalFieldDecompressor::~OptionalFieldDecompressor (void) {
	gzclose(file);
}

string OptionalFieldDecompressor::getRecord (void) {
	if (records.size() == recordCount) {
		records.clear();
		recordCount = 0;
		importRecords();
	}
	return records[recordCount++];
}

void OptionalFieldDecompressor::importRecords (void) {
	char *c = new char[blockSize * RECORD_AVG];
	size_t size = gzread(file, c, blockSize * RECORD_AVG);

	string tmp = "";
	size_t pos = 0;
	while (pos < size) {
		if (c[pos] != 0)
			tmp += c[pos];
		else {
			records.push_back(tmp);
			tmp="";
		}
		pos++;
	}
	if (pos == size && c[pos - 1] != 0) {
		unsigned char t = c[pos - 1];
		while (t) { // border case
			gzread(file, &t, 1);
			tmp += t;
		}
		records.push_back(tmp);
	}

	LOG("%d quality scores are loaded", records.size());
	delete[] c;
}



/*
of.resize(256 * 256, 0);

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
    */