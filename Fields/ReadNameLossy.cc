#include "ReadNameLossy.h"
#include <tr1/unordered_map>
using namespace std;

ReadNameLossyCompressor::ReadNameLossyCompressor (int blockSize):
	GenericCompressor<uint32_t, GzipCompressionStream<6> > (blockSize)
{
}

ReadNameLossyCompressor::~ReadNameLossyCompressor (void) {
}

std::tr1::unordered_map<string, int32_t> mmap;
void ReadNameLossyCompressor::addRecord (const string &s) {
	auto it = mmap.find(s);
	if (it == mmap.end()) {
		int num = mmap.size() + 1;
		it = mmap.insert(make_pair(s, num)).first;
	}

	GenericCompressor::addRecord(it->second);
}

