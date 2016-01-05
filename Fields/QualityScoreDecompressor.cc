#include "QualityScore.h"
using namespace std;

QualityScoreDecompressor::QualityScoreDecompressor (int blockSize):
	StringDecompressor<QualityDecompressionStream>(blockSize) 
{
	sought = 0;
	switch (optQuality) {
		case 0:
			break;
		case 1:
			streams[0] = make_shared<SAMCompStream<QualRange>>();
			sought = 1;
			break;
		case 2:
			streams[0] = make_shared<ArithmeticOrder2DecompressionStream<QualRange>>();
			break;
		default:
			throw DZException("Not implemented");
			break;
	}
	const char* qualities[] = { "default", "samcomp", "arithmetic" };
	LOG("Using quality mode %s", qualities[optQuality]);
	if (optNoQual)
		sought = 2;
}

void QualityScoreDecompressor::setIndexData (uint8_t *in, size_t in_size) 
{
	streams[0]->setCurrentState(in, in_size);
	if (sought) 
		sought = 2;
}

string QualityScoreDecompressor::getRecord (size_t record, size_t seq_len, int flag) 
{
	ZAMAN_START(QualityScoreGet);

	if (sought == 2) {
		string s = "";
		for (int i = 0; i < seq_len; i++)
			s += (char)64;
		ZAMAN_END(QualityScoreGet);
		return s;
	}

	string s = string(StringDecompressor<QualityDecompressionStream>::getRecord(record));
	if (s == "") {
		ZAMAN_END(QualityScoreGet);
		return "*";
	}

	for (size_t i = 0; i < s.size(); i++)
		s[i] += offset - 1;
    char c = s[s.size() - 1];
    while (s.size() < seq_len)
		s += c;
    if (flag & 0x10) for (size_t i = 0; i < seq_len / 2; i++)
		swap(s[i], s[seq_len - i - 1]);

	ZAMAN_END(QualityScoreGet);
    return s;
}

void QualityScoreDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) 
		return;
	if (sought == 2)
		return;
	
	ZAMAN_START(QualityScoreImport);
	offset = *in++;
	StringDecompressor<QualityDecompressionStream>::importRecords(in, in_size - 1);
	ZAMAN_END(QualityScoreImport);
}

