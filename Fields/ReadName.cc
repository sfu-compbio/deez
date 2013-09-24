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
	StringCompressor<GzipCompressionStream<6> >(blockSize),
	token(0)
{
	indexStream = new GzipCompressionStream<6>();
}

ReadNameCompressor::~ReadNameCompressor (void) {
	delete indexStream;
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
	//DEBUG("Token character: [%c]", token);
}

void ReadNameCompressor::addTokenizedName (const string &rn, Array<uint8_t> &content, Array<uint8_t> &index) {
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
					content.add(tk[j]);
				content.add(0);
				index.add(i + 5 * MAX_TOKEN);
			}
			else if (e < (1 << 8)) {
				content.add(e);
				index.add(i);
			}
			else if (e < (1 << 16)) {
				DO(2) content.add(e & 0xff), e >>= 8;
				index.add(i + 1 * MAX_TOKEN);
			}
			else if (e < (1 << 24)) {
				DO(3) content.add(e & 0xff), e >>= 8;
				index.add(i + 2 * MAX_TOKEN);
			}
			else if (e < (1ll << 32)) {
				DO(4) content.add(e & 0xff), e >>= 8;
				index.add(i + 3 * MAX_TOKEN);
			}
			else {
				DO(8) content.add(e & 0xff), e >>= 8;
				index.add(i + 4 * MAX_TOKEN);
			}
			prevTokens[i] = tk;
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

	Array<uint8_t> buffer;
	Array<uint8_t> indices;	

	for (size_t i = 0; i < k; i++) {
		if (!token) {
			detectToken(this->records[i]);
			indices.add(token);
		}
		addTokenizedName(this->records[i], buffer, indices);
	}

	size_t s = indexStream->compress((uint8_t*)indices.data(), 
		indices.size(), out, out_offset + sizeof(size_t));
	memcpy(out.data() + out_offset, &s, sizeof(size_t));
	
	size_t sn = stream->compress(buffer.data(), buffer.size(), 
		out, out_offset + s + 2 * sizeof(size_t));
	memcpy(out.data() + out_offset + s + sizeof(size_t), &sn, sizeof(size_t));
	out.resize(out_offset + sn + s + 2 * sizeof(size_t));
	//// 
	this->records.remove_first_n(k);
}

ReadNameDecompressor::ReadNameDecompressor (int blockSize):
	StringDecompressor<GzipDecompressionStream> (blockSize),
	token(0)
{
	indexStream = new GzipDecompressionStream();
}

ReadNameDecompressor::~ReadNameDecompressor (void) {
	delete indexStream;
}

void ReadNameDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) return;

	assert(recordCount == records.size());

	// decompress
	Array<uint8_t> index, content;
	index.resize( 51000000 );
	content.resize( 51000000 );

	size_t in1 = *((size_t*)in);
	size_t s = indexStream->decompress(in + sizeof(size_t), in1, index, 0);
	
	int in2 = *((size_t*)(in + in1 + sizeof(size_t)));
	size_t s2 = stream->decompress(in + in1 + 2 * sizeof(size_t), in2, content, 0);

	string tokens[MAX_TOKEN];
	size_t ic = 0, cc = 0;

	if (!token) 
		token = index.data()[ic++];

	while (ic < s) {
		int T, t, prevT = 0;
		while (1) {
			t = (T = index.data()[ic++]) % MAX_TOKEN;
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
					DO(T + 1) e |= content.data()[cc++] << (8 * _);
					//assert(tokens[t].size() == 0);
					tokens[t] = "";
					while (e) tokens[t] = char('0' + e % 10) + tokens[t], e /= 10;
					break;
				}
				case 5: 
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
		for(int i = 1; i < t; i++)
			tokens[0] += token + tokens[i];
		records.add(tokens[0]);
	}
}