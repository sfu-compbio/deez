#include "Order3Model.h"

#include <time.h>
#include <sys/time.h>

static int64_t time_ticks = 0;
int64_t _TIME_() {
	struct timeval t;
	gettimeofday(&t,0);
	return (t.tv_sec*1000000ll+t.tv_usec);
}


Order3Model::Order3Model (void):
	alphabetSize(256),
	seenSoFar(0)
{
	freq3 = new uint64_t[alphabetSize * alphabetSize];
	freq4 = new uint64_t[alphabetSize * alphabetSize * alphabetSize];
//	freq4_active = new uint64_t[alphabetSize * alphabetSize * alphabetSize];
	hifreq = new uint64_t[alphabetSize * alphabetSize * alphabetSize];
	lofreq = new uint64_t[alphabetSize * alphabetSize * alphabetSize];

	for (int i = 0; i < alphabetSize * alphabetSize * alphabetSize; i++)
		freq4[i] = 1;

	update();
}

Order3Model::~Order3Model (void) {
	delete[] freq3;
	delete[] freq4;
//	delete[] freq4_active;
	delete[] hifreq;
	delete[] lofreq;

	DEBUG("Rescale time %'lld", time_ticks);
}

void Order3Model::update (void) {
	DEBUG("rescaling... %llu", seenSoFar);

	int64_t T = _TIME_();

	for (int i = 0; i < alphabetSize; i++)
		for (int j = 0; j < alphabetSize; j++) {
			int off = (i * alphabetSize + j) * alphabetSize;

			hifreq[off + 0] = freq4[off + 0];
			for (int l = 1; l < alphabetSize; l++) {
				hifreq[off + l] = hifreq[off + l - 1] + freq4[off + l];
				assert(hifreq[off + l] < (1ll<<31));
			}

			freq3[off / alphabetSize] = hifreq[off + alphabetSize - 1];
	
			int p = -1;
			for (int l = 0; l < alphabetSize; l++)
				if (freq4[off + l]) {
					if (p != -1) lofreq[off + l] = hifreq [off + p];
					p = l;
				}
		}

	time_ticks += _TIME_() - T;
}

bool Order3Model::active (void) const {
	return seenSoFar >= 1;
}

uint32_t Order3Model::getLow (unsigned char c) const {
	return lofreq[(c0 * alphabetSize + c1) * alphabetSize + c];
}

uint32_t Order3Model::getHigh (unsigned char c) const {
	return hifreq[(c0 * alphabetSize + c1) * alphabetSize + c];
}

uint32_t Order3Model::getSpan (void) const {
	return freq3[c0 * alphabetSize + c1];
}

void Order3Model::add (unsigned char c) {
	if (active()) {
		freq4[(c0 * alphabetSize + c1) * alphabetSize + c]++;
	}

	c0 = c1;
	c1 = c;
	seenSoFar++;

	if (seenSoFar == 1000 || seenSoFar == 10000 || seenSoFar == 1000000 ||
			seenSoFar % 100000000 == 0)
		update();
}

unsigned char Order3Model::find (uint32_t cnt) {
	uint32_t off = (c0 * alphabetSize + c1) * alphabetSize;
	for (size_t i = 0; i < alphabetSize; i++)
		if (/**freq4[off + i] &&**/ cnt >= lofreq[off + i] && cnt < hifreq[off + i]) {
			if (char(i)<0)throw;
			return i;
		}
	throw "AC find failed";
}
