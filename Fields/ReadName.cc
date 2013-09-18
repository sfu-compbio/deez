#include "ReadName.h"
using namespace std;

/*
 * Three types of names:
 * SRR:
 	* SRR00000000	.	1
 * Solexa
	* EAS139		: 136	: FC706VJ	: 2 	: 2104		: 15343	: 197393 
 * Illumina
	* HWUSI-EAS100R	: 6		: 73		: 941	: 197393	#0/1
*/

#define DO(x) for(int _=0;_<x;_++)

ReadNameCompressor::ReadNameCompressor (int blockSize):
	indexStream(blockSize * 10),
	contentStream(blockSize * 50),
	token(0)
{
}

ReadNameCompressor::~ReadNameCompressor (void) {
}

void ReadNameCompressor::detectToken (const string &rn) {
	char al[256] = { 0 };
	for (int i = 0; i < rn.size(); i++)
		al[rn[i]]++;
	// only . and :
	if (al['.'] > al[':'])
		token = '.';
	else
		token = ':';
	indexStream.addRecord(token);
	DEBUG("Token character: [%c]", token);
}

void ReadNameCompressor::addTokenizedName (const string &rn) {
	int tokens[MAX_TOKEN], tc = 0;
	tokens[tc++] = 0;
	for (size_t i = 0; i < rn.size(); i++)
		if (rn[i] == this->token && tc < MAX_TOKEN)
			tokens[tc++] = i + 1;
	tokens[tc] = rn.size() + 1;
	for (int i = 0; i < tc; i++) {
		string tk = rn.substr(tokens[i], tokens[i + 1] - tokens[i] - 1);
		if (tk != prevTokens[i]) {
			uint64_t e = atol(tk.c_str());
			if (e == 0) {
				for (int j = 0; j < tk.size(); j++) 
					contentStream.addRecord(tk[j]);
				contentStream.addRecord(0);
				indexStream.addRecord(i + 5 * MAX_TOKEN);
			}
			else if (e < (1 << 8)) {
				contentStream.addRecord(e);
				indexStream.addRecord(i);
			}
			else if (e < (1 << 16)) {
				DO(2) contentStream.addRecord(e & 0xff), e >>= 8;
				indexStream.addRecord(i + 1 * MAX_TOKEN);
			}
			else if (e < (1 << 24)) {
				DO(3) contentStream.addRecord(e & 0xff), e >>= 8;
				indexStream.addRecord(i + 2 * MAX_TOKEN);
			}
			else if (e < (1ll << 32)) {
				DO(4) contentStream.addRecord(e & 0xff), e >>= 8;
				indexStream.addRecord(i + 3 * MAX_TOKEN);
			}
			else {
				DO(8) contentStream.addRecord(e & 0xff), e >>= 8;
				indexStream.addRecord(i + 4 * MAX_TOKEN);
			}
			prevTokens[i] = tk;
		}
	}
	indexStream.addRecord(6 * MAX_TOKEN + tc); // how many tokens
}

void ReadNameCompressor::addRecord (const string &rn) {
	if (!token)
		detectToken(rn);
	addTokenizedName(rn);
}

void ReadNameCompressor::outputRecords (Array<uint8_t> &output) {
	output.resize(0);
	size_t sz = 0;
	output.add((uint8_t*)&sz, sizeof(size_t));
	indexStream.appendRecords(output);
	sz = output.size() - sizeof(size_t);
	*(size_t*)output.data() = sz;
	contentStream.appendRecords(output);
}

ReadNameDecompressor::ReadNameDecompressor (int blockSize):
	indexStream(blockSize * 10),
	contentStream(blockSize * 50),
	token(0)
{
}

ReadNameDecompressor::~ReadNameDecompressor (void) {
}

bool ReadNameDecompressor::hasRecord (void) {
	return indexStream.hasRecord();
}

void ReadNameDecompressor::importRecords (uint8_t *in, size_t in_size) {
	size_t sz = *(size_t*)in;
	indexStream.importRecords(in + sizeof(size_t), sz);
	contentStream.importRecords(in + sizeof(size_t) + sz, in_size - sz - sizeof(size_t));

	if (!token) {
		assert(hasRecord());
		token = indexStream.getRecord();
	}
}

void ReadNameDecompressor::extractTokenizedName (string &s) {
	string tokens[MAX_TOKEN];
	
	int T, t, prevT = 0;
	while (1) {
		t = (T = indexStream.getRecord()) % MAX_TOKEN;
		while (prevT < t)
			tokens[prevT] = prevTokens[prevT], prevT++;
		T /= MAX_TOKEN;
		if (T >= 6) 
			break;
		switch (T) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4: {
				uint64_t e = 0;
				if (T == 4) T += 3;
				DO(T + 1) e |= contentStream.getRecord() << (8 * _);
				assert(tokens[t].size() == 0);
				while (e) tokens[t] = char('0' + e % 10) + tokens[t], e /= 10;
				break;
			}
			case 5: {
				char c;
				while ((c = contentStream.getRecord())) tokens[t] += c;
				break;
			}
			default:
				throw DZException("Read name data corrupted");
		}
		prevTokens[t] = tokens[t];
		prevT = t;
	}

	s = tokens[0];
	for(int i = 1; i < t; i++)
		s += token + tokens[i];
}


string ReadNameDecompressor::getRecord (void) {
	assert(hasRecord());

	string s;
	extractTokenizedName(s);
	return s;
}

