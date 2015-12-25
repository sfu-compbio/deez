#include "ReadName.h"
using namespace std;

ReadNameCompressor::ReadNameCompressor(void):
	StringCompressor<GzipCompressionStream<6>>()
{
	streams.resize(Fields::ENUM_COUNT);
	for (int i = 0; i < streams.size(); i++)
		streams[i] = make_shared<GzipCompressionStream<6>>();
	memset(prevCharTokens, 0, MAX_TOKEN);
}

void ReadNameCompressor::printDetails(void)
{
	LOG("  Index     : %'20lu", streams[Fields::INDEX]->getCount());
	LOG("  Content   : %'20lu", streams[Fields::CONTENT]->getCount());
	LOG("  Paired    : %'20lu", streams[Fields::PAIRED]->getCount());
}

void ReadNameCompressor::outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k, const Array<PairedEndInfo> &pairedEndInfos) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	ZAMAN_START(ReadNameOutput);

	Array<uint8_t> buffer(this->totalSize, MB);
	Array<uint8_t> indices(k * 10);	
	Array<uint8_t> paired(k);	

	for (size_t i = 0; i < k; i++) {
		size_t rnLen = strlen(records[i].getReadName());
		if (pairedEndInfos[i].bit == PairedEndInfo::Bits::LOOK_BACK) {
			indices.add(6 * MAX_TOKEN);
			if (pairedEndInfos[i].tlen < (1 << 16)) {
				uint16_t sz16 = pairedEndInfos[i].tlen;
				assert(sz16 > 0);
				paired.add((uint8_t*)&sz16, sizeof(uint16_t));
			} else {
				uint16_t sz16 = 0;
				paired.add((uint8_t*)&sz16, sizeof(uint16_t));	
				paired.add((uint8_t*)&pairedEndInfos[i].tlen, sizeof(uint32_t));	
			}
		} else {
			addTokenizedName(records[i].getReadName(), 
				rnLen,
				buffer, indices);
		}
		this->totalSize -= rnLen + 1;
	}

	compressArray(streams[Fields::INDEX], indices, out, out_offset);
	compressArray(streams[Fields::CONTENT], buffer, out, out_offset);
	compressArray(streams[Fields::PAIRED], paired, out, out_offset);
	
	for (int i = 0; i < MAX_TOKEN; i++) {
		prevTokens[i] = "";
		prevCharTokens[i] = 0;
	}

	ZAMAN_END(ReadNameOutput);
}

void ReadNameCompressor::getIndexData (Array<uint8_t> &out) 
{
	out.resize(0);
	streams[Fields::CONTENT]->getCurrentState(out);
}

void ReadNameCompressor::addTokenizedName (const char *rn, size_t rnLen, Array<uint8_t> &content, Array<uint8_t> &index) 
{	
	ZAMAN_START(Tokenize);

	int tokens[MAX_TOKEN], tc = 0;
	char charTokens[MAX_TOKEN] = { 0 };
	tokens[tc++] = 0;

	bool specialSRRCase = false;
	for (size_t i = 0; i < rnLen; i++)
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
		string tk = string(rn + tokens[i], tokens[i + 1] - tokens[i] - 1);
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
	
	ZAMAN_END(Tokenize);
}
