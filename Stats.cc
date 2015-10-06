#include "Stats.h"
#include "Fields/Sequence.h"
#include <algorithm>
using namespace std;

Stats::Stats():
	reads(0)
{
	fill(flags, flags + FLAGCOUNT, 0);
}

Stats::Stats (Array<uint8_t> &in, uint32_t magic) {
	uint8_t *pin = in.data();
	fileName = string((char*)pin);
	pin += fileName.size() + 1;

	reads = *(size_t*)pin; pin += 8;
	memcpy((uint8_t*)flags, pin, FLAGCOUNT * 8);
	pin += FLAGCOUNT * sizeof(size_t);

	while (pin < in.data() + in.size()) {
		string chr = string((char*)pin);
		pin += chr.size() + 1;
		if ((magic & 0xff) >= 0x11) {
			string fn = string((char*)pin);
			pin += fn.size() + 1;
			string md5 = string((char*)pin);
			pin += md5.size() + 1;
			chromosomes[chr] = {chr, md5, fn, 0, 0};
		}

		size_t len = *(size_t*)pin; 
		pin += 8;
		chromosomes[chr].len = len;
	}
}

Stats::~Stats() {
}

void Stats::addRecord (uint16_t flag) {
	flags[flag]++;
	reads++;
}

void Stats::writeStats (Array<uint8_t> &out, SequenceCompressor *seq) {
	out.resize(0);
	out.add((uint8_t*)(fileName.c_str()), fileName.size() + 1);
	out.add((uint8_t*)&reads, sizeof(size_t));
	out.add((uint8_t*)flags, FLAGCOUNT * sizeof(size_t));
	
	foreach (p, seq->reference.chromosomes) {
		out.add((uint8_t*)(p->second.chr.c_str()), p->second.chr.size() + 1);
		// v1.1 begin
		auto s = seq->reference.getChromosomeInfo(p->first);
		out.add((uint8_t*)(p->second.filename.c_str()), p->second.filename.size() + 1);
		out.add((uint8_t*)(p->second.md5.c_str()), p->second.md5.size() + 1);
		// v1.1 end
		out.add((uint8_t*)&(p->second.len), sizeof(size_t));
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