#ifndef Stats_H
#define Stats_H

#include "Common.h"
#include "Reference.h"
#include <map>
#include <string>

class SequenceCompressor;

class Stats {
public:
	static const int FLAGCOUNT = (1 << 16) - 1;
	
private:
	size_t flags[FLAGCOUNT];
	size_t reads;

public:
	std::string fileName;
	std::map<std::string, Reference::Chromosome> chromosomes;

public:
	Stats();
	Stats(Array<uint8_t> &in, uint32_t);
	~Stats();

public:
	void addRecord (uint16_t flag);
	void writeStats (Array<uint8_t> &out, SequenceCompressor*);
	size_t getStats (int flag);

	size_t getReadCount() { return reads; }
	size_t getChromosomeCount() { return chromosomes.size(); }
	size_t getFlagCount(int i) { return flags[i]; }
};

#endif // Stats_H