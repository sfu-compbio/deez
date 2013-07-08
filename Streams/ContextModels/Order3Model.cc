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
	alphabetSize(128),
	seenSoFar(0)
{
	freq4  = new uint64_t[alphabetSize * alphabetSize * alphabetSize];
	lofreq = new uint64_t[alphabetSize * alphabetSize * alphabetSize];

	for (int i = 0; i < alphabetSize * alphabetSize * alphabetSize; i++)
		freq4[i] = 1;

	update();
}

Order3Model::~Order3Model (void) {
	delete[] freq4;
	delete[] lofreq;

//	DEBUG("Rescale time %'lld", time_ticks);
}

void Order3Model::update (void) {
//	DEBUG("rescaling... %llu", seenSoFar);

//	int64_t T = _TIME_();

	for (int i = 0; i < alphabetSize; i++)
		for (int j = 0; j < alphabetSize; j++) {
			int off = (i * alphabetSize + j) * alphabetSize;

			lofreq[off] = 0;
			for (int l = 1; l < alphabetSize; l++)
				lofreq[off + l] = lofreq[off + l - 1] + freq4[off + l - 1];

			uint64_t f3 = lofreq[off + alphabetSize - 1] + freq4[off + alphabetSize - 1];
			while (f3 >= (1ll << 31)) {
				fprintf(stderr,"$");
				for (int l = 0; l < alphabetSize; l++)
					freq4[off + l] -= (freq4[off + l] >> 1);
				lofreq[off] = 0;
				for (int l = 1; l < alphabetSize; l++)
					lofreq[off + l] = lofreq[off + l - 1] + freq4[off + l - 1];
				f3 = lofreq[off + alphabetSize - 1] + freq4[off + alphabetSize - 1];
			}
		}
	fprintf(stderr,"\n");

//	time_ticks += _TIME_() - T;
}

bool Order3Model::active (void) const {
	return seenSoFar >= 2;
}

uint32_t Order3Model::getLow (unsigned char c) const {
	return lofreq[(c0 * alphabetSize + c1) * alphabetSize + c];
}

uint32_t Order3Model::getFreq (unsigned char c) const {
	assert(c < alphabetSize - 1);
	assert(lofreq[(c0 * alphabetSize + c1) * alphabetSize + c + 1] > lofreq[(c0 * alphabetSize + c1) * alphabetSize + c]);
	return 	lofreq[(c0 * alphabetSize + c1) * alphabetSize + c + 1] -
			lofreq[(c0 * alphabetSize + c1) * alphabetSize + c];
}

uint32_t Order3Model::getSpan (void) const {
	return 	lofreq[(c0 * alphabetSize + c1) * alphabetSize + alphabetSize - 1] + 
			freq4[(c0 * alphabetSize + c1) * alphabetSize + alphabetSize - 1];
}

void Order3Model::add (unsigned char c) {
	if (active())
		freq4[(c0 * alphabetSize + c1) * alphabetSize + c]++;

	c0 = c1;
	c1 = c;
	seenSoFar++;

	if (seenSoFar == 1000 || seenSoFar == 10000 || seenSoFar == 1000000 ||
			seenSoFar % 100000000 == 0)
		update();
}

unsigned char Order3Model::find (uint32_t cnt) {
	uint32_t off = (c0 * alphabetSize + c1) * alphabetSize;
	for (size_t i = 0; i < alphabetSize; i++) {
		if (cnt < lofreq[off + i + 1])
			return i;
	}
	throw DZException("AC find failed, %lu vs %lu", cnt, getSpan());
}
