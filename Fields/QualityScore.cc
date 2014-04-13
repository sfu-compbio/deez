#include "QualityScore.h"
using namespace std;

QualityScoreCompressor::QualityScoreCompressor (int blockSize):
	StringCompressor<QualityCompressionStream>(blockSize),
	statMode(true)
{
	memset(lossy, 0, 128 * sizeof(int));
	memset(stat, 0, 128 * sizeof(int));
	switch (optQuality) {
		case 0:
			break;
		case 1:
			delete stream;
			stream = new SAMCompStream<QualRange>();
			break;
		case 2:
			delete stream;
			stream = new MyCompressionStream<QualRange>();
			break;
	}
	const char* qualities[] = { "default", "samcomp", "test" };
	LOG("Using quality mode %s", qualities[optQuality]);
}

QualityScoreCompressor::~QualityScoreCompressor (void) {

}

double QualityScoreCompressor::phredScore (char c, int offset) {
	return pow(10, (offset - c) / 10.0);
}

bool statSort (const pair<char, int> &a, const pair<char, int> &b) {
	return (a.second > b.second || (a.second == b.second && a.first < b.first));
}

char QualityScoreCompressor::calculateOffset (void) {
	for (int i = 33; i < 64; i++) {
		if (i < 59 && stat[i]) return 33;
		if (stat[i]) return 59;
	}
	return 64;
}

void QualityScoreCompressor::calculateLossyTable (int percentage) {
	if (!offset)
		throw DZException("Offset not calculated");

	// calculate replacements
	for (int c = 0; c < 128; c++)
		lossy[c] = c;

	// if percentage is 0% then keep original qualities
	if (!percentage)
		return;

	vector<char> alreadyAssigned(128, 0);
	// Discard all elements with >30% error
	for (char hlimit = offset; 100 * phredScore(hlimit, offset) > 30; hlimit++) {
		lossy[hlimit] = offset;
		alreadyAssigned[hlimit] = 1;
	}

	// sort stats by maximum value
	pair<char, int> bmm[128];
	for (int i = 0; i < 128; i++)
		bmm[i] = make_pair(i, stat[i]);
	sort(bmm, bmm + 128, statSort);

	// obtain replacement table
	double p = percentage / 100.0;
	for (int i = 0; i < 128 && bmm[i].second; i++) {
		if (!alreadyAssigned[bmm[i].first] 
			&& stat[bmm[i].first] >= stat[bmm[i].first - 1] 
			&& stat[bmm[i].first] >= stat[bmm[i].first + 1]) 
		{
			double er = phredScore(bmm[i].first, offset);
			double total = er;
			char left = bmm[i].first, 
				 right = bmm[i].first;
			for (int c = bmm[i].first - 1; c >= 0; c--) {
				total += phredScore(c, offset);
				if (alreadyAssigned[c] || total / (bmm[i].first - c + 1) > er + (er * p)) {
					left = c + 1;
					break;
				}
			}
			total = er;
			for (int c = bmm[i].first + 1; c < 128; c++) {
				total += phredScore(c, offset);
				if (alreadyAssigned[c] || total / (c - bmm[i].first + 1) < er - (er * p)) {
					right = c - 1;
					break;
				}
			}
			for (char c = left; c <= right; c++) {
				lossy[c] = bmm[i].first;
				alreadyAssigned[c] = 1;
			}
		}
	}
}

void QualityScoreCompressor::lossyTransform (string &qual) {
	/*if (statMode) */ for (size_t i = 0; i < qual.size(); i++)
		stat[qual[i]]++;
	if (!optLossy) return;
	if (!statMode) for (size_t i = 0; i < qual.size(); i++)
		qual[i] = lossy[qual[i]];
}

void QualityScoreCompressor::addRecord (string qual, int flag) {
	if (qual == "*") {
		StringCompressor<QualityCompressionStream>::addRecord(string());
		return;
	}

	size_t sz = qual.size();
	if (flag & 0x10) for (size_t j = 0; j < sz / 2; j++)
		swap(qual[j], qual[sz - j - 1]);
	lossyTransform(qual);

	if (sz >= 2) {
		sz -= 2;
		while (sz && qual[sz] == qual[qual.size() - 1])
			sz--;
		sz += 2;
	}
	qual = qual.substr(0, sz);
	assert(qual.size() > 0);
	StringCompressor<QualityCompressionStream>::addRecord(qual);
}

void QualityScoreCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!offset) {
		offset = calculateOffset();
	//	LOG("Quality offset is %d", offset);
	}
	if (optLossy && statMode) {
		calculateLossyTable(optLossy);
		statMode = false;	
		for (size_t i = 0; i < k; i++) 
			for (int j = 0; j < this->records[i].size(); j++)
				this->records[i][j] = lossy[this->records[i][j]];
	} 

//	delete stream;
//	stream = new QualityCompressionStream();
	out.add(offset); out_offset++;
//	for (int i = 0; i < k; i++)  LOG("%s",this->records[i].c_str());
	for (int i = 0; i < k; i++) 
		for (int j = 0; j < this->records[i].size(); j++) {
			char &c = this->records[i][j];
			if (c < offset)
				throw DZException("Quality scores out of range with L offset %d [%c]", offset, c);
			c = (c - offset) + 1;
			if (c >= QualRange)
				throw DZException("Quality scores out of range with R offset %d [%c]", offset, c + offset - 1);
		}
	StringCompressor<QualityCompressionStream>::outputRecords(out, out_offset, k);
	memset(stat, 0, 128 * sizeof(int));
	offset = 0;
}

QualityScoreDecompressor::QualityScoreDecompressor (int blockSize):
	StringDecompressor<QualityDecompressionStream>(blockSize) 
{
	switch (optQuality) {
		case 0:
			break;
		case 1:
			delete this->stream;
			this->stream = new SAMCompStream<QualRange>();
			break;
		case 2:
			//delete stream;
			//stream = new MyCompressionStream<QualRange>();
			break;
	}
	const char* qualities[] = { "default", "samcomp", "test" };
	LOG("Using quality mode %s", qualities[optQuality]);
}

QualityScoreDecompressor::~QualityScoreDecompressor (void) {
}

string QualityScoreDecompressor::getRecord (size_t seq_len, int flag) {
	assert(hasRecord());
	
	string s = string(StringDecompressor<QualityDecompressionStream>::getRecord());
	if (s == "")
		return "*";

	for (size_t i = 0; i < s.size(); i++)
		s[i] += offset - 1;
    char c = s[s.size() - 1];
    while (s.size() < seq_len)
		s += c;
    if (flag & 0x10) for (size_t i = 0; i < seq_len / 2; i++)
		swap(s[i], s[seq_len - i - 1]);
    return s;
}

void QualityScoreDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) 
		return;
	offset = *in;
	StringDecompressor<QualityDecompressionStream>::importRecords(in + 1, in_size - 1);
}

