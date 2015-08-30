#include "ReadNameLossy.h"

#if __cplusplus <= 199711L
#include <tr1/unordered_map>
std::tr1::unordered_map<string, int32_t> mmap;
#else
#include <unordered_map>
std::unordered_map<string, int32_t> mmap;
#endif

using namespace std;

ReadNameLossyCompressor::ReadNameLossyCompressor (int blockSize):
	GenericCompressor<uint32_t, GzipCompressionStream<6> > (blockSize)
{
}

ReadNameLossyCompressor::~ReadNameLossyCompressor (void) {
}

void ReadNameLossyCompressor::addRecord (const string &s) {
	auto it = mmap.find(s);
	if (it == mmap.end()) {
		int num = mmap.size() + 1;
		it = mmap.insert(make_pair(s, num)).first;
	}

	GenericCompressor::addRecord(it->second);
}

