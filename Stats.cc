#include "Stats.h"
#include <algorithm>
using namespace std;

Stats::Stats():
	reads(0)
{
	fill(flags, flags + FLAGCOUNT, 0);
}

Stats::Stats (Array<uint8_t> &in) {
	uint8_t *pin = in.data();

	reads = *(size_t*)pin; pin += 8;
	memcpy((uint8_t*)flags, pin, FLAGCOUNT * 8);
	pin += FLAGCOUNT * sizeof(size_t);

	while (pin < in.data() + in.size()) {
		string chr = string((char*)pin);
		pin += chr.size() + 1;
		size_t len = *(size_t*)pin; 
		pin += 8;
		chromosomes[chr] = len;
	}
}

Stats::~Stats() {

}

void Stats::addRecord (uint16_t flag) {
	flags[flag]++;
	reads++;
}

void Stats::addChromosome (const string &chr, size_t len) {
	if (chromosomes.find(chr) != chromosomes.end())
		throw DZException("Chromosome %s already registered!", chr.c_str());
	chromosomes[chr] = len;
}

void Stats::writeStats (Array<uint8_t> &out) {
	out.resize(0);
	out.add((uint8_t*)&reads, sizeof(size_t));
	out.add((uint8_t*)flags, FLAGCOUNT * sizeof(size_t));
	foreach (p, chromosomes) {
		out.add((uint8_t*)(p->first.c_str()), p->first.size() + 1);
		out.add((uint8_t*)&(p->second), sizeof(size_t));
	}
}

size_t Stats::getStats (int flag) {
	int result = 0;
	if (flag > 0) {
		for (int i = 0; i < FLAGCOUNT; i++)
			if ((i & flag) == flag) result += flags[i];
	}
	else if (flag < 0) {
		flag = -flag;
		for (int i = 0; i < FLAGCOUNT; i++)
			if ((i & flag) == 0) result += flags[i];
	}
	return result;
}