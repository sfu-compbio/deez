#include "ReadNameLossy.h"
using namespace std;

ReadNameLossyCompressor::ReadNameLossyCompressor (int blockSize):
	ReadNameCompressor(blockSize)
{
	throw DZException("Not implemented");
}

ReadNameLossyCompressor::~ReadNameLossyCompressor (void) {
}

void ReadNameLossyCompressor::addRecord (const string &s) {
	auto it = mmap.find(s);
	if (it == mmap.end()) {
		int num = mmap.size() + 1;
		it = mmap.insert(make_pair(s, num)).first;
	}

	GenericCompressor::addRecord(inttostr(it->second));
}

