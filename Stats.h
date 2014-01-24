#ifndef Stats_H
#define Stats_H

#include "Common.h"
#include <map>
#include <string>

class Stats {
public:
	static const int FLAGCOUNT = (1 << 16) - 1;
	
private:
	size_t flags[FLAGCOUNT];
	size_t reads;
	std::map<std::string, size_t> chromosomes;
	// CRCs?

public:
	Stats();
	Stats(Array<uint8_t> &in);
	~Stats();

public:
	void addRecord (uint16_t flag);
	void addChromosome (const std::string &chr, size_t len);
	void writeStats (Array<uint8_t> &out);
	size_t getStats (int flag);

	size_t getReadCount() { return reads; }
	size_t getChromosomeCount() { return chromosomes.size(); }
	size_t getFlagCount(int i) { return flags[i]; }
};

#endif // Stats_H