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

ReadNameCompressor::ReadNameCompressor (int blockSize):
	StringCompressor<GzipCompressionStream<6> >(blockSize)
//	token(0)
{
	indexStream = new GzipCompressionStream<6>();
	memset(prevCharTokens, 0, MAX_TOKEN);
}

ReadNameCompressor::~ReadNameCompressor (void) {
	delete indexStream;
}
/*
void ReadNameCompressor::detectToken (const string &rn) {
	char al[256] = { 0 };
	for (int i = 0; i < rn.size(); i++)
		al[rn[i]]++;
	// only .:_
	token = '.';
	if (al[':'] > al[token])
		token = ':';
	if (al['_'] > al[token])
		token = '_';
	//DEBUG("Token character: [%c]", token);
}
*/
void ReadNameCompressor::getIndexData (Array<uint8_t> &out) {
	out.resize(0);
	//out.add(token);
	stream->getCurrentState(out);
}

void encodeUInt(uint64_t e, Array<uint8_t> &index, Array<uint8_t> &content, int i) {
//	content.add((uint8_t*)&e, 8);
//	index.add(i);

	if (e < (1 << 8)) {
		content.add(e);
		index.add(i);
	}
	else if (e < (1 << 16)) {
		REPEAT(2) content.add(e & 0xff), e >>= 8;
		index.add(i + 1 * MAX_TOKEN);
	}
	else if (e < (1 << 24)) {
		REPEAT(3) content.add(e & 0xff), e >>= 8;
		index.add(i + 2 * MAX_TOKEN);
	}
	else if (e < (1ll << 32)) {
		REPEAT(4) content.add(e & 0xff), e >>= 8;
		index.add(i + 3 * MAX_TOKEN);
	}
	else {
		REPEAT(8) content.add(e & 0xff), e >>= 8;
		index.add(i + 4 * MAX_TOKEN);
	}
}

uint64_t decodeUInt (int T, Array<uint8_t> &content, size_t &cc) {
//	assert(T==0);
	uint64_t e = 0; //*(uint64_t*)(content.data() + cc);
//	cc += 8;
	
	if (T == 4) T += 3;
	REPEAT(T + 1) e |= content.data()[cc++] << (8 * _);
	return e;	
}

void ReadNameCompressor::addTokenizedName (const string &rn, Array<uint8_t> &content, Array<uint8_t> &index) {
	int tokens[MAX_TOKEN], tc = 0;
	char charTokens[MAX_TOKEN] = { 0 };
	tokens[tc++] = 0;

	bool specialSRRCase = false;
	for (size_t i = 0; i < rn.size(); i++)
		if (tc < MAX_TOKEN) {
			if (rn[i] < '0' || (rn[i] > '9' && rn[i] < 'A') || (rn[i] > 'Z' && rn[i] < 'a')) {
				charTokens[tc - 1] = rn[i];
				tokens[tc++] = i + 1;
			}
			// SRX ERX SRA etc
			else if (i == 3 && tc == 1 && isalpha(rn[i - 1]) && rn[i] >= '1' && rn[i] <= '9') {
				tokens[tc++] = i;
				specialSRRCase = true;
			}
		}

/*
	LOG("----");
	for (int i = 0; i < tc; i++) {
				string tk = rn.substr(tokens[i], tokens[i + 1] - tokens[i] - 1);
				if (i == 0 && specialSRRCase) tk += rn[tk.size()];

		LOG("%s . %d [%c]", 		
			tk.c_str(),
			charTokens[i], charTokens[i]);
	}	
*/
	tokens[tc] = rn.size() + 1;
	for (int i = 0; i < tc; i++) {
		string tk = rn.substr(tokens[i], tokens[i + 1] - tokens[i] - 1);
		if (i == 0 && specialSRRCase) tk += rn[tk.size()];
		if (tk != prevTokens[i] || charTokens[i] != prevCharTokens[i]) {
			char *p;
			long e = strtol(tk.c_str(), &p, 10);
			if (*p || e < 0 || !tk.size() || tk[0] == '0') {
				for (int j = 0; j < tk.size(); j++) 
					content.add(tk[j]);
				content.add(0);
				index.add(i + 5 * MAX_TOKEN);			
			}				
			else {
			    encodeUInt(e, index, content, i);
			}
			prevTokens[i] = tk;
		}
	// +
		if (charTokens[i] != prevCharTokens[i]) {
			encodeUInt(charTokens[i], index, content, i);
			prevCharTokens[i] = charTokens[i];
		}
	}
	index.add(6 * MAX_TOKEN + tc); // how many tokens
}

void ReadNameCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	Array<uint8_t> buffer(this->totalSize, MB);
	Array<uint8_t> indices(k * 10);	

	for (size_t i = 0; i < k; i++) {
	//	if (!token) {
	//		detectToken(this->records[i]);
	//		indices.add(token);
	//	}
		addTokenizedName(this->records[i], buffer, indices);
		this->totalSize -= this->records[i].size() + 1;
	}

	compressArray(indexStream, indices, out, out_offset);
	compressArray(stream, buffer, out, out_offset);
	for (int i = 0; i < MAX_TOKEN; i++) {
		prevTokens[i] = "";
		prevCharTokens[i] = 0;
	}
	this->records.remove_first_n(k);
}

ReadNameDecompressor::ReadNameDecompressor (int blockSize):
	StringDecompressor<GzipDecompressionStream> (blockSize)
//	token(0)
{
	indexStream = new GzipDecompressionStream();
}

ReadNameDecompressor::~ReadNameDecompressor (void) {
	delete indexStream;
}

void ReadNameDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) return;

	//assert(recordCount == records.size());

	Array<uint8_t> index;
	size_t s1 = decompressArray(indexStream, in, index);
__debug_fwrite(index.data(), 1, index.size(), ____debug_file[__DC++]);
	Array<uint8_t> content;
	decompressArray(stream, in, content);
__debug_fwrite(content.data(), 1, content.size(), ____debug_file[__DC++]);

	string tokens[MAX_TOKEN];
	size_t ic = 0, cc = 0;

//	if (!token) 
//		token = index.data()[ic++];

	records.resize(0);
	string prevTokens[MAX_TOKEN];
	char prevCharTokens[MAX_TOKEN] = { 0 };
	char charTokens[MAX_TOKEN] = { 0 };

	while (ic < s1) {
		int prevTokenID = -1;
		int T, t, prevT = 0;
		while (1) {
			t = (T = index.data()[ic++]) % MAX_TOKEN;
			
			if (t == prevTokenID) {
				charTokens[t] = decodeUInt(0, content, cc);
				prevCharTokens[t] = charTokens[t];
		//		LOGN("+[%c]", charTokens[t]);
				continue;
			}
			prevTokenID = t;

			while (prevT < t) {
				tokens[prevT] = prevTokens[prevT];
				charTokens[prevT] = prevCharTokens[prevT]; 
				prevT++;
			}
			T /= MAX_TOKEN;
			if (T >= 6)  
				break;
			switch (T) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 4: {
					uint64_t e = decodeUInt(T, content, cc);
					tokens[t] = "";
					if (!e) tokens[t] = "0";
					while (e) tokens[t] = char('0' + e % 10) + tokens[t], e /= 10;
					break;
				}
				case 5: 
					tokens[t] = "";
					while (content.data()[cc])  
						tokens[t] += content.data()[cc++];
					cc++;
					break;
				default:
					throw DZException("Read name data corrupted");
			}
			prevTokens[t] = tokens[t];
			prevT = t;
		}
		if (t > 1 && charTokens[0] == 0 && tokens[0].size() == 3 // == '.' //&& tokens[0].size() && !isdigit(tokens[0][tokens[0].size() - 1]) 
				&& isalpha(tokens[0][2]) && tokens[1][0] >= '1' && tokens[1][0] <= '9') 
		{
			tokens[0] += tokens[1];
			for (int i = 2; i < t; i++)
				tokens[0] += charTokens[i - 1] + tokens[i];
		}
		else for (int i = 1; i < t; i++)
			tokens[0] += charTokens[i - 1] + tokens[i];
		records.add(tokens[0]);
	}

	recordCount = 0;
}

void ReadNameDecompressor::setIndexData (uint8_t *in, size_t in_size) {
//	token = *in++; in_size--;
	stream->setCurrentState(in, in_size);
}