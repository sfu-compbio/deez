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

void ReadNameCompressor::getIndexData (Array<uint8_t> &out) {
	out.resize(0);
	out.add(token);
	stream->getCurrentState(out);
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

	Array<uint8_t> buffer(this->totalSize, MB);
	Array<uint8_t> indices(k * 10);	

	for (size_t i = 0; i < k; i++) {
		if (!token) {
			detectToken(this->records[i]);
			indices.add(token);
		}
		addTokenizedName(this->records[i], buffer, indices);
		this->totalSize -= this->records[i].size() + 1;
	}

	size_t s = 0;
	if (indices.size()) s = indexStream->compress((uint8_t*)indices.data(), 
		indices.size(), out, out_offset + 2 * sizeof(size_t));
	// size of compressed
	*(size_t*)(out.data() + out_offset) = s;
	// size of uncompressed
	*(size_t*)(out.data() + out_offset + sizeof(size_t)) = indices.size();

	out_offset += s + 2 * sizeof(size_t);
	out.resize(out_offset);
	
	s = 0;
	if (buffer.size()) s = stream->compress(buffer.data(), buffer.size(), 
		out, out_offset + 2 * sizeof(size_t));
	// size of compressed
	*(size_t*)(out.data() + out_offset) = s;
	// size of uncompressed
	*(size_t*)(out.data() + out_offset + sizeof(size_t)) = buffer.size();
	
	out_offset += s + 2 * sizeof(size_t);
	out.resize(out_offset);

	//// 
	for (int i = 0; i < MAX_TOKEN; i++)
		prevTokens[i] = "";
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
	size_t in1 = *(size_t*)in; in += sizeof(size_t);
	size_t de1 = *(size_t*)in; in += sizeof(size_t);
	Array<uint8_t> index;
	index.resize(de1);
	size_t s1 = 0;
	if (in1) s1 = indexStream->decompress(in, in1, index, 0);
	assert(s1 == de1);
	in += in1;

	size_t in2 = *(size_t*)in; in += sizeof(size_t);
	size_t de2 = *(size_t*)in; in += sizeof(size_t);
	Array<uint8_t> content;
	content.resize(de2);
	size_t s2 = 0;
	if (in2) s2 = stream->decompress(in, in2, content, 0);
	assert(s2 == de2);
	in += in2;

	string tokens[MAX_TOKEN];
	size_t ic = 0, cc = 0;

	if (!token) 
		token = index.data()[ic++];

	records.resize(0);
	string prevTokens[MAX_TOKEN];
	while (ic < s1) {
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

	recordCount = 0;
}

void ReadNameDecompressor::setIndexData (uint8_t *in, size_t in_size) {
	token = *in++; in_size--;
	stream->setCurrentState(in, in_size);
}