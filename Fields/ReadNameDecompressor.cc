#include "ReadName.h"
using namespace std;

ReadNameDecompressor::ReadNameDecompressor (int blockSize):
	StringDecompressor<GzipDecompressionStream>(blockSize),
	paired(blockSize)
{
	streams.resize(ReadNameCompressor::Fields::ENUM_COUNT);
	for (int i = 0; i < streams.size(); i++)
		streams[i] = make_shared<GzipDecompressionStream>();
}

ReadNameDecompressor::~ReadNameDecompressor (void) 
{
}

void ReadNameDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	ZAMAN_START(ReadNameImport);

	Array<uint8_t> index;
	size_t s1 = decompressArray(streams[ReadNameCompressor::Fields::INDEX], in, index);
	Array<uint8_t> content;
	decompressArray(streams[ReadNameCompressor::Fields::CONTENT], in, content);
	
	Array<uint8_t> pp;
	decompressArray(streams[ReadNameCompressor::Fields::PAIRED], in, pp);
	uint8_t *p = pp.data();
	paired.resize(0);

	string tokens[MAX_TOKEN];
	size_t ic = 0, cc = 0;

	records.resize(0);
	string prevTokens[MAX_TOKEN];
	char prevCharTokens[MAX_TOKEN] = { 0 };
	char charTokens[MAX_TOKEN] = { 0 };

	while (ic < s1) {
		int prevTokenID = -1;
		int T, t, prevT = 0;

		while (ic < s1) {
			t = (T = index.data()[ic++]) % MAX_TOKEN;

			if (T == 6 * MAX_TOKEN) {
				int32_t tlen = *(uint16_t*)p; p += sizeof(uint16_t);
				if (!tlen) 
					tlen = *(uint32_t*)p, p += sizeof(uint32_t);
				paired.add(tlen);
				records.add("");
				continue;
			}
			
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
		records.add(tokens[0]);
	}

	ZAMAN_END(ReadNameImport);
}

uint32_t ReadNameDecompressor::getPaired(size_t i)
{
	return paired[i];
}

void ReadNameDecompressor::setIndexData (uint8_t *in, size_t in_size) 
{
	streams[ReadNameCompressor::Fields::CONTENT]->setCurrentState(in, in_size);
}