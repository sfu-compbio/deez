#include "OptionalField.h"
using namespace std;
//
//OptionalFieldCompressor::OptionalFieldCompressor (int blockSize) {
//	stream = new GzipCompressionStream<6>();
//	keyStream = new GzipCompressionStream<6>();
//	fieldIdx.resize(256 * 256, 0);
//	records.reserve(blockSize);
//}
//
//OptionalFieldCompressor::~OptionalFieldCompressor (void) {
//	delete keyStream;
//	delete stream;
//}
//
//void OptionalFieldCompressor::addRecord (const string &rec) {
//	vector<short> keys;
//	for (int i = 0; i < rec.size(); i++) {
//		int j = i;
//		while (j < rec.size() && rec[j] != '\t') 
//			j++;
//		if (j - i < 5)
//			throw DZException("SAM optional tag is invalid!");
//
//		short idx = rec[i] * 256 + rec[i + 1];
//		if (!fieldIdx[idx]) {
//			fieldKey.push_back(rec.substr(i, 2));
//			fieldType.push_back(rec[i + 3]);
//			fieldData.push_back(vector<string>());
//			fieldIdx[idx] = fieldKey.size();
//		}
//		fieldData[fieldIdx[idx] - 1].push_back(rec.substr(i + 5, j - i - 5));
//		keys.push_back(fieldIdx[idx] - 1);
//		i = j;
//	}
//	records.push_back(keys);
//}
//
//void OptionalFieldCompressor::outputRecords (std::vector<char> &output) {
//	if (records.size() > 0) {
//		vector<char> c;
//		
//		/**
//		 * Block contents:
//		 * {size_t} size
//		 * {...}    keys   --> {short}  size
//		 *                     {short*} array
//		 * {...}    values --> {short}  size
//		 *                     {...}    tags   --> {char.2} key
//		 *                                         {char}   type
//		 *                                         {char*}  value
//		 **/
//		
//		size_t s = records.size();
//		c.insert(c.end(), (char*)&s, (char*)(&s + sizeof(size_t)));
//
//		for (size_t i = 0; i < records.size(); i++) {
//			short sz = records[i].size();
//			c.insert(c.end(), (char*)&sz, (char*)(&sz + sizeof(short)));
//			keyStream->compress(&records[i][0], sz * sizeof(short), c);
//		}
//
//		short sz = fieldKey.size();
//		c.insert(c.end(), (char*)&sz, (char*)(&sz + sizeof(short)));
//		for (size_t i = 0; i < fieldKey.size(); i++) {
//			stream->compress((void*)fieldKey[i].c_str(), 2 * sizeof(char), c);
//			stream->compress(&fieldType[i], sizeof(char), c);
//
//			int l = fieldData[i].size();
//			c.insert(c.end(), (char*)&l, (char*)(&l + sizeof(int)));
//			for (int j = 0; j < l; j++)
//				stream->compress((void*)fieldData[i][j].c_str(), fieldData[i][j].size() + 1, c);
//			fieldData[i].clear();
//		}
//		records.erase(records.begin(), records.end());
//	}
//}
//
//OptionalFieldDecompressor::OptionalFieldDecompressor (int blockSize):
//	recordCount(0) 
//{
//	stream = new GzipDecompressionStream();
//	keyStream = new GzipDecompressionStream();
//	records.reserve(blockSize);
//}
//
//OptionalFieldDecompressor::~OptionalFieldDecompressor (void) {
//	delete stream;
//	delete keyStream;
//}
//
//string OptionalFieldDecompressor::getRecord (void) {
//	assert(hasRecord());
//	
//	const vector<short> &v = records[recordCount++];
//	string result = "";
//	for (int i = 0; i < v.size(); i++) {
//		if (i) result += "\t";
//		result += fieldKey[v[i]] + ":" + fieldType[v[i]] + ":";
//		result += fieldData[v[i]][0];
//		fieldData[v[i]].pop_front();
//	}
//	return result;
//}
//
//bool OptionalFieldDecompressor::hasRecord (void) {
//	return recordCount < records.size();
//}
//
//void OptionalFieldDecompressor::importRecords (const std::vector<char> &input) {
//	/**
//	* Block contents:
//	* {size_t} size
//	* {...}    keys   --> {short}  size
//	*                     {short*} array
//	* {...}    values --> {short}  size
//	*                     {...}    tags   --> {char.2} key
//	*                                         {char}   type
//	*                                         {char*}  value
//	**/
//		
//	const char *ptr = &input[0];
//	
//	size_t s = * (size_t*)ptr;
//	ptr += sizeof(size_t);
//	for (size_t i = 0; i < s; i++) {
//		short sz = * (short*)ptr; 
//		ptr += sizeof(short);
//		
//		records.push_back(vector<short>(sz));
//		keyStream->decompress(ptr, sz, 
//		c.insert(c.end(), (char*)&sz, (char*)(&sz + sizeof(short)));
//		keyStream->compress(&records[i][0], sz * sizeof(short), c);
//	}
//
//	short sz = fieldKey.size();
//	c.insert(c.end(), (char*)&sz, (char*)(&sz + sizeof(short)));
//	for (size_t i = 0; i < fieldKey.size(); i++) {
//		stream->compress((void*)fieldKey[i].c_str(), 2 * sizeof(char), c);
//		stream->compress(&fieldType[i], sizeof(char), c);
//
//		int l = fieldData[i].size();
//		c.insert(c.end(), (char*)&l, (char*)(&l + sizeof(int)));
//		for (int j = 0; j < l; j++)
//			stream->compress((void*)fieldData[i][j].c_str(), fieldData[i][j].size() + 1, c);
//		fieldData[i].clear();
//	}
//	records.erase(records.begin(), records.end());
//
//
//
//	short s;
//	for (int i = 0; i < blockSize; i++) {
//		short sz;
//		sz = &input[0] + i * 
//		if (!gzread(keyFile, &s, sizeof(short)))
//			break;
//		records.push_back(vector<short>(s));
//		gzread(keyFile, &records[i][0], s * sizeof(short));
//	}
//
//	gzread(valueFile, &s, sizeof(short));
//	fieldKey.resize(s);
//	fieldType.resize(s);
//	fieldData.resize(s);
//	char buf[2];
//	for (int i = 0; i < s; i++) {
//		gzread(valueFile, buf, 2 * sizeof(char));
//		fieldKey[i] = string(buf, 2);
//		gzread(valueFile, buf, sizeof(char)); 
//		fieldType[i] = buf[0];
//
//		int l;
//		gzread(valueFile, &l, sizeof(int));
//		fieldData[i].resize(l);
//		for (int j = 0; j < l; j++) {
//			// do only strings for now...
//			fieldData[i][j] = "";
//			while (1) {
//				gzread(valueFile, buf, sizeof(char));
//				if (!buf[0]) break;
//				fieldData[i][j] += buf[0];
//			}
//		}
//	}
//
//	LOG("%d optional fields are loaded", records.size());
//}