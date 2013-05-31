#include "OptionalField.h"
using namespace std;

static const char *LOG_PREFIX = "<OF>";
static const int  RECORD_AVG 	= 100;

OptionalFieldCompressor::OptionalFieldCompressor (const string &filename, int blockSize) {
	string name1(filename + ".ofk.dz");
	keyFile = gzopen(name1.c_str(), "wb6");
	if (keyFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	name1 = filename + ".ofv.dz";
	valueFile = gzopen(name1.c_str(), "wb6");
	if (valueFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	fieldIdx.resize(256 * 256, 0);
}

OptionalFieldCompressor::~OptionalFieldCompressor (void) {
	gzclose(keyFile);
	gzclose(valueFile);
}

void OptionalFieldCompressor::addRecord (const string &rec) {
	vector<short> keys;
	for (int i = 0; i < rec.size(); i++) {
		int j = i;
		while (j < rec.size() && rec[j] != '\t') 
			j++;
		if (j - i < 5)
			throw DZException("SAM optional tag is invalid!");

		short idx = rec[i] * 256 + rec[i + 1];
		if (!fieldIdx[idx]) {
			fieldKey.push_back(rec.substr(i, 2));
			fieldType.push_back(rec[i + 3]);
			fieldData.push_back(vector<string>());
			fieldIdx[idx] = fieldKey.size();
		}
		fieldData[fieldIdx[idx] - 1].push_back(rec.substr(i + 5, j - i - 5));
		keys.push_back(fieldIdx[idx] - 1);
		i = j;
	}
	records.push_back(keys);
}

void OptionalFieldCompressor::outputRecords (void) {
	if (records.size() > 0) {
		short s;
		for (int i = 0; i < records.size(); i++) {
			s = records[i].size();
			gzwrite(keyFile, &s, sizeof(short));
			gzwrite(keyFile, &records[i][0], records[i].size() * sizeof(short));
		}

		s = fieldKey.size();
		gzwrite(valueFile, &s, sizeof(short));
		for (int i = 0; i < fieldKey.size(); i++) {
			gzwrite(valueFile, fieldKey[i].c_str(), 2 * sizeof(char)); // 2
			gzwrite(valueFile, &fieldType[i], sizeof(char)); 			  // 1
			int l = fieldData[i].size();
			gzwrite(valueFile, &l, sizeof(int));
			for (int j = 0; j < l; j++) {
				// do only strings for now...
				gzwrite(valueFile, fieldData[i][j].c_str(), fieldData[i][j].size() + 1);
			}
			fieldData[i].clear();
		}
	}
	records.clear();
}

OptionalFieldDecompressor::OptionalFieldDecompressor (const string &filename, int bs): 
	blockSize (bs), recordCount (0) 
{
	string name1(filename + ".ofk.dz");
	keyFile = gzopen(name1.c_str(), "rb");
	if (keyFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());

	name1 = filename + ".ofv.dz";
	valueFile = gzopen(name1.c_str(), "rb");
	if (valueFile == Z_NULL)
		throw DZException("Cannot open the file %s", name1.c_str());
}

OptionalFieldDecompressor::~OptionalFieldDecompressor (void) {
	gzclose(keyFile);
	gzclose(valueFile);
}

string OptionalFieldDecompressor::getRecord (void) {
	if (records.size() == recordCount) {
		records.clear();
		recordCount = 0;
		importRecords();
	}

	const vector<short> &v = records[recordCount++];
	string result = "";
	for (int i = 0; i < v.size(); i++) {
		if (i) result += "\t";
		result += fieldKey[v[i]] + ":" + fieldType[v[i]] + ":";
		result += fieldData[v[i]][0];
		fieldData[v[i]].pop_front();
	}
	return result;
}

void OptionalFieldDecompressor::importRecords (void) {
	records.clear();
	short s;
	for (int i = 0; i < blockSize; i++) {
		if (!gzread(keyFile, &s, sizeof(short)))
			break;
		records.push_back(vector<short>(s));
		gzread(keyFile, &records[i][0], s * sizeof(short));
	}

	/// idx?
	gzread(valueFile, &s, sizeof(short));
	fieldKey.resize(s);
	fieldType.resize(s);
	fieldData.resize(s);
	char buf[2];
	for (int i = 0; i < s; i++) {
		gzread(valueFile, buf, 2 * sizeof(char));
		fieldKey[i] = string(buf, 2);
		gzread(valueFile, buf, sizeof(char)); 
		fieldType[i] = buf[0];

		int l;
		gzread(valueFile, &l, sizeof(int));
		fieldData[i].resize(l);
		for (int j = 0; j < l; j++) {
			// do only strings for now...
			fieldData[i][j] = "";
			while (1) {
				gzread(valueFile, buf, sizeof(char));
				if (!buf[0]) break;
				fieldData[i][j] += buf[0];
			}
		}
	}

	LOG("%d optional fields are loaded", records.size());
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
