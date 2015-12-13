#include "ReadName.h"
using namespace std;

ReadNameDecompressor::ReadNameDecompressor (int blockSize):
	StringDecompressor<GzipDecompressionStream> (blockSize)
{
	indexStream = new GzipDecompressionStream();
}

ReadNameDecompressor::~ReadNameDecompressor (void) 
{
	delete indexStream;
}

void ReadNameDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	ZAMAN_START(Decompress_ReadName);

	Array<uint8_t> index;
	size_t s1 = decompressArray(indexStream, in, index);
	Array<uint8_t> content;
	decompressArray(stream, in, content);

	string tokens[MAX_TOKEN];
	size_t ic = 0, cc = 0;

	records.resize(0);
	string prevTokens[MAX_TOKEN];
	char prevCharTokens[MAX_TOKEN] = { 0 };
	char charTokens[MAX_TOKEN] = { 0 };

	while (ic < s1) {
		int prevTokenID = -1;
		int T, t, prevT = 0;
		while (1) {
			t = (T = index.data()[ic++]) % MAX_TOKEN;

			if (t == 6 * MAX_TOKEN) {
				records.add("");
				continue;
			}
			
			if (t == prevTokenID) {
				charTokens[t] = unpackInteger(0, content, cc);
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

	recordCount = 0;

	ZAMAN_END(Decompress_ReadName);
}

void ReadNameDecompressor::setIndexData (uint8_t *in, size_t in_size) 
{
	stream->setCurrentState(in, in_size);
}