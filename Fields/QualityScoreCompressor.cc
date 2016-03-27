#include "QualityScore.h"
#include <cmath>
using namespace std;

#include "../Streams/Stream.h"	
#include "../Streams/BzipStream.h"	

QualityScoreCompressor::QualityScoreCompressor(void):
	StringCompressor<QualityCompressionStream>(),
	statMode(true),
	offset(0)
{
	memset(lossy, 0, 128 * sizeof(int));
	memset(stat, 0, 128 * sizeof(int));
	switch (optQuality) {
		case 0:
			break;
		case 1:
			streams[0] = make_shared<SAMCompStream<QualRange>>();
			break;
		case 2:
			streams[0] = make_shared<ArithmeticOrder2CompressionStream<QualRange>>();
			break;
		default:
			throw DZException("Invalid stream specified");
			break;
	}
	const char* qualities[] = { "default", "samcomp", "arithmetic" };
	LOG("Using quality mode %s", qualities[optQuality]);
}

size_t QualityScoreCompressor::shrink(char *qual, size_t len, int flag)
{
	if (!len || (len == 1 && qual[0] == '*')) {
		*qual = 0;
		return 0;
	}

	if (flag & 0x10) for (size_t j = 0; j < len / 2; j++)
		swap(qual[j], qual[len - j - 1]);

	ssize_t sz = len;
	if (sz >= 2) {
		sz -= 2;
		while (sz && qual[sz] == qual[len - 1])
			sz--;
		sz += 2;
	}
	qual[sz] = 0;
	assert(sz > 0);
	return sz;
}

int QualityScoreCompressor::getOffset()
{
	return offset;
}

void QualityScoreCompressor::updateOffset(int *st)
{
	if (offset) return;
	for (int i = 0; i < 128; i++)
		stat[i] += st[i];
}

void QualityScoreCompressor::calculateOffset()
{
	if (offset) return;
	
	offset = 64;
	for (int i = 33; i < 64; i++) {
		if (i < 59 && stat[i]) {
			offset = 33;
			break;
		}
		if (stat[i]) {
			offset = 59;
			break;
		}
	}
	
	if (optLossy && statMode) {
		calculateLossyTable(optLossy);
		statMode = false;	
	} 
}

void QualityScoreCompressor::offsetRecord (Record &rc)
{
	assert(offset);
	char *qual = (char*)rc.getQuality();
	lossyTransform(qual, strlen(qual));
	while (*qual) {
		if (*qual < offset)
			throw DZException("Quality scores out of range with L offset %d [%c]", offset, *qual);
		*qual = (*qual - offset) + 1;
		if (*qual >= QualRange)
			throw DZException("Quality scores out of range with R offset %d [%c]", offset, *qual + offset - 1);
		qual++;
	}
}

void QualityScoreCompressor::outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	ZAMAN_START(QualityScoreOutput);
	
	out.add(offset); out_offset++;

	Array<uint8_t> buffer(totalSize, MB);
	for (size_t i = 0; i < k; i++) {
		const char *q = records[i].getQuality();
		while (*q)
			buffer.add(*q), q++;
		buffer.add(0);
		totalSize -= (q - records[i].getQuality()) + 1;
	}
	compressArray(streams.front(), buffer, out, out_offset);
	memset(stat, 0, 128 * sizeof(int));
	offset = 0;

	ZAMAN_END(QualityScoreOutput);
}

double QualityScoreCompressor::phredScore (char c, int offset) 
{
	return pow(10, (offset - c) / 10.0);
}

bool statSort (const pair<char, int> &a, const pair<char, int> &b) 
{
	return (a.second > b.second || (a.second == b.second && a.first < b.first));
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

void QualityScoreCompressor::lossyTransform (char *qual, size_t len) 
{
	if (!optLossy) 
		return;
	if (!statMode) for (size_t i = 0; i < len; i++)
		qual[i] = lossy[qual[i]];
}
