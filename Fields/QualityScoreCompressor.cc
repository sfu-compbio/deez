#include "QualityScore.h"
using namespace std;

QualityScoreCompressor::QualityScoreCompressor (int blockSize):
	StringCompressor<QualityCompressionStream>(blockSize),
	statMode(true),
	offset(0)
{
	memset(lossy, 0, 128 * sizeof(int));
	memset(stat, 0, 128 * sizeof(int));
	switch (optQuality) {
		case 0:
			break;
		case 1:
			delete stream;
			stream = new SAMCompStream<AC, QualRange>();
			break;
		default:
			throw DZException("Invalid stream specified");
			break;
	}
	const char* qualities[] = { "default", "samcomp", "test" };
	LOG("Using quality mode %s", qualities[optQuality]);
}

QualityScoreCompressor::~QualityScoreCompressor (void) 
{
}

void QualityScoreCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	ZAMAN_START(Compress_QualityScore);

	if (!offset) {
		offset = calculateOffset();
	}
	if (optLossy && statMode) {
		calculateLossyTable(optLossy);
		statMode = false;	
		for (size_t i = 0; i < k; i++) 
			for (int j = 0; j < this->records[i].size(); j++)
				this->records[i][j] = lossy[this->records[i][j]];
	} 

	out.add(offset); out_offset++;
	for (int i = 0; i < k; i++) {
		for (int j = 0; j < this->records[i].size(); j++) {
			char &c = this->records[i][j];
			if (c < offset)
				throw DZException("Quality scores out of range with L offset %d [%c]", offset, c);
			c = (c - offset) + 1;
			if (c >= QualRange)
				throw DZException("Quality scores out of range with R offset %d [%c]", offset, c + offset - 1);
		}
	}
	StringCompressor<QualityCompressionStream>::outputRecords(out, out_offset, k);
	memset(stat, 0, 128 * sizeof(int));
	offset = 0;

	ZAMAN_END(Compress_QualityScore);
}

double QualityScoreCompressor::phredScore (char c, int offset) 
{
	return pow(10, (offset - c) / 10.0);
}

bool statSort (const pair<char, int> &a, const pair<char, int> &b) 
{
	return (a.second > b.second || (a.second == b.second && a.first < b.first));
}

char QualityScoreCompressor::calculateOffset (void) 
{
	for (int i = 33; i < 64; i++) {
		if (i < 59 && stat[i]) 
			return 33;
		if (stat[i]) 
			return 59;
	}
	return 64;
}

void QualityScoreCompressor::calculateLossyTable (int percentage) 
{
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

void QualityScoreCompressor::lossyTransform (string &qual) 
{
	for (size_t i = 0; i < qual.size(); i++)
		stat[qual[i]]++;
	if (!optLossy) 
		return;
	if (!statMode) for (size_t i = 0; i < qual.size(); i++)
		qual[i] = lossy[qual[i]];
}

void QualityScoreCompressor::shrink(size_t i, int flag)
{
	assert(i < records.size());
	if (!records[i].size())
		return;

	size_t sz = records[i].size();
	if (flag & 0x10) for (size_t j = 0; j < sz / 2; j++)
		swap(records[i][j], records[i][sz - j - 1]);
	lossyTransform(records[i]);

	if (sz >= 2) {
		sz -= 2;
		while (sz && records[i][sz] == records[i][records[i].size() - 1])
			sz--;
		sz += 2;
	}
	records[i] = records[i].substr(0, sz);
	assert(records[i].size() > 0);
}

void QualityScoreCompressor::addRecord (const string &qual) 
{
	ZAMAN_START(Compress_QualityScore_Add);
	if (qual == "*") {
		StringCompressor<QualityCompressionStream>::addRecord(string());
	} else {
		StringCompressor<QualityCompressionStream>::addRecord(qual);
	}
	ZAMAN_END(Compress_QualityScore_Add);
}
