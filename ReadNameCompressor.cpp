/// 786

#include <bits/stdc++.h>
#include <sys/time.h>
#include "Array.h"
#include "Streams/Stream.h"
#include "Streams/Stream.cc"
#ifdef RBZ2
#include "Streams/BzipStream.h"
#else
#include "Streams/GzipStream.h"
#endif
#include "Common.cc"
using namespace std;

const int MAX_TOKEN = 20;
const int BLOCK_SIZE = 10000000;

template<typename T>
size_t compressArray (shared_ptr<CompressionStream> c, Array<T> &in) 
{
	Array<uint8_t> out;
	size_t s = 0;
	out.resize(2 * sizeof(size_t));
	if (in.size()) s = c->compress((uint8_t*)in.data(), 
		in.size() * sizeof(T), out, 2 * sizeof(size_t));
	*(size_t*)(out.data()) = s;
	*(size_t*)(out.data() + sizeof(size_t)) = in.size() * sizeof(T);
	fwrite(out.data(), 1, s + 2 * sizeof(size_t), stdout);
	in.resize(0);
	return s + 2 * sizeof(size_t);
	//WARN("-- %d %d", s, in.size() * sizeof(T));
}

size_t decompressArray (shared_ptr<DecompressionStream> d, Array<uint8_t> &out) 
{
	size_t in_sz;
	size_t de_sz;
	if (fread(&in_sz, sizeof(size_t), 1, stdin) != 1) 
		return 0;
	fread(&de_sz, sizeof(size_t), 1, stdin);

	//WARN(">> %d %d", in_sz, de_sz);
	
	out.resize(de_sz);
	size_t sz = 0;
	if (in_sz) {
		Array<uint8_t> in(in_sz);
		fread(in.data(), 1, in_sz, stdin);
		sz = d->decompress(in.data(), in_sz, out, 0);
	}
	assert(sz == de_sz);
	return sz;
}

void addTokenizedName (const string &rn, char *prevCharTokens, string *prevTokens, Array<uint8_t> &content, Array<uint8_t> &index) 
{	
	int tokens[MAX_TOKEN], tc = 0;
	char charTokens[MAX_TOKEN] = { 0 };
	tokens[tc++] = 0;

	bool specialSRRCase = false;
	size_t rnLen = 0;
	for (size_t i = 0; i < rn.size(); i++, rnLen++)
		if (tc < MAX_TOKEN) {
			if (rn[i] < '0' || (rn[i] > '9' && rn[i] < 'A') || (rn[i] > 'Z' && rn[i] < 'a')) {
				charTokens[tc - 1] = rn[i];
				tokens[tc++] = i + 1;
			} else if (i == 3 && tc == 1 && isalpha(rn[i - 1]) && rn[i] >= '1' && rn[i] <= '9') { // SRX ERX SRA etc
				tokens[tc++] = i;
				specialSRRCase = true;
			}
		}
	
	tokens[tc] = rnLen + 1;
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
			} else {
			    int sz = packInteger(e, content);
			    index.add(i + sz * MAX_TOKEN);
			}
			prevTokens[i] = tk;
		}
		if (charTokens[i] != prevCharTokens[i]) {
			int sz = packInteger(charTokens[i], content);
			index.add(i + sz * MAX_TOKEN);
			prevCharTokens[i] = charTokens[i];
		}
	}
	index.add(6 * MAX_TOKEN + tc); // how many tokens
}

void decompressHelper(size_t s1, Array<uint8_t> &index, Array<uint8_t> &content, Array<string> &output) {
	string tokens[MAX_TOKEN];
	size_t ic = 0, cc = 0;

	string prevTokens[MAX_TOKEN];
	char prevCharTokens[MAX_TOKEN] = { 0 };
	char charTokens[MAX_TOKEN] = { 0 };

	while (ic < s1) {
		int prevTokenID = -1;
		int T, t, prevT = 0;

		while (ic < s1) {
			t = (T = index.data()[ic++]) % MAX_TOKEN;
			
			if (t == prevTokenID) {
				charTokens[t] = unpackInteger(0, content, cc);
				prevCharTokens[t] = charTokens[t];
				continue;
			}
			prevTokenID = t;

			while (prevT < t) {
				tokens[prevT] = prevTokens[prevT];
				charTokens[prevT] = prevCharTokens[prevT]; 
				prevT++;
			}
			T /= MAX_TOKEN;
			if (T >= 6)  {
				break;
			}
			switch (T) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 4: {
					uint64_t e = unpackInteger(T, content, cc);
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
		output.add(tokens[0]);
	}
}

bool decompress () 
{
	#ifdef RBZ2
		#define DSTREAM BzipDecompressionStream
	#else
		#define DSTREAM GzipDecompressionStream
	#endif

	auto indexStream = make_shared<DSTREAM>();
	auto bufferStream = make_shared<DSTREAM>();

	auto indexStreamComment = make_shared<DSTREAM>();
	auto bufferStreamComment = make_shared<DSTREAM>();

	Array<uint8_t> index, indexComment;
	size_t s1 = decompressArray(indexStream, index);
	if (s1 == 0) return false;
	
	Array<uint8_t> content, contentComment;
	decompressArray(bufferStream, content);

	size_t s1C = decompressArray(indexStreamComment, indexComment);
	decompressArray(bufferStreamComment, contentComment);
	
	Array<string> output(BLOCK_SIZE); 
	decompressHelper(s1, index, content, output);

	Array<string> outputComment(BLOCK_SIZE); 
	decompressHelper(s1C, indexComment, contentComment, outputComment);

	for (int i = 0; i < output.size(); i++) {
		fputc('@', stdout);
		if (outputComment[i].size() == 0)
			puts(output[i].c_str());
		else {
			fputs(output[i].c_str(), stdout);
			fputc(' ', stdout);
			puts(outputComment[i].c_str());
		}
	}
	
	return true;
}

int main (int argc, char **argv)
{
	if (argc > 1) {
		WARN("dec");
		while (decompress()) ;
		exit(0);
	}

	ios::sync_with_stdio(0);

	std::string prevTokens[MAX_TOKEN];
	char prevCharTokens[MAX_TOKEN];
	std::string prevTokensComment[MAX_TOKEN];
	char prevCharTokensComment[MAX_TOKEN];

	for (int i = 0; i < MAX_TOKEN; i++) {
		prevTokens[i] = prevTokensComment[i] = "";
		prevCharTokens[i] = prevCharTokensComment[i] = 0;
	}


	#ifdef RBZ2
		#define ISTREAM BzipCompressionStream
	#else
		#define ISTREAM GzipCompressionStream<6>
	#endif

	auto indexStream = make_shared<ISTREAM>();
	auto bufferStream = make_shared<ISTREAM>();

	auto indexStreamComment = make_shared<ISTREAM>();
	auto bufferStreamComment = make_shared<ISTREAM>();

	Array<uint8_t> buffer(BLOCK_SIZE * 50, 100 * MB);
	Array<uint8_t> indices(BLOCK_SIZE * 10);	
	Array<uint8_t> bufferComment(BLOCK_SIZE * 50, 100 * MB);
	Array<uint8_t> indicesComment(BLOCK_SIZE * 10);	
	size_t blockCount = 0, count = 0;
	string s;

	size_t readC = 0, commentC = 0;

	while (getline(cin, s)) {
		int comment = s.size();
		for (int i = 0; i < s.size(); i++)
			if (s[i] == ' ') { comment = i; break; }

		// if (count/BLOCK_SIZE==3) {
		// 	WARN("%s", s.c_str());
		// }
		addTokenizedName(s.substr(1,comment-1), prevCharTokens, prevTokens, buffer, indices);
		if(comment==s.size())
			addTokenizedName("", prevCharTokensComment, prevTokensComment, bufferComment, indicesComment);
		else
			addTokenizedName(s.substr(comment+1), prevCharTokensComment, prevTokensComment, bufferComment, indicesComment);
		

		count++; blockCount++;
		if (blockCount == BLOCK_SIZE) {
			WARN("block %d in", count/BLOCK_SIZE);
			readC += compressArray(indexStream, indices);
			readC += compressArray(bufferStream, buffer);
			commentC += compressArray(indexStreamComment, indicesComment);
			commentC += compressArray(bufferStreamComment, bufferComment);
			blockCount = 0;
			for (int i = 0; i < MAX_TOKEN; i++) {
				prevTokens[i] = prevTokensComment[i] = "";
				prevCharTokens[i] = prevCharTokensComment[i] = 0;
			}
			WARN("block %d", count/BLOCK_SIZE);
		}

		getline(cin, s);
		getline(cin, s);
		getline(cin, s);		
	}

	// fclose(XF);

	readC += compressArray(indexStream, indices);
	readC += compressArray(bufferStream, buffer);
	commentC += compressArray(indexStreamComment, indicesComment);
	commentC += compressArray(bufferStreamComment, bufferComment);

	WARN("%d reads, %d comments, %d total", readC, commentC, readC+commentC);
	WARN("%d reads, %d blocks", count, count / BLOCK_SIZE + 1);

	return 0;
}
