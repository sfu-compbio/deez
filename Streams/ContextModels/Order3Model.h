#ifndef Order3Model_H
#define Order3Model_H

#include "../../Common.h"

class Order3Model/*: public CodingModel*/ {
	uint64_t *freq4;
	uint64_t *lofreq;

	size_t alphabetSize;
	size_t seenSoFar;

	char c0;
	char c1;

public:
	Order3Model (void);
	~Order3Model (void);

private:
	void update (void);

public:
	bool active (void) const;
	uint32_t getLow (unsigned char c) const;
	uint32_t getFreq (unsigned char c) const;
	uint32_t getSpan (void) const;
	void add (unsigned char c);
	unsigned char find (uint32_t cnt);
};

#endif
