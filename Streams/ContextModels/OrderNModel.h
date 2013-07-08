#ifndef OrderNModel_H
#define OrderNModel_H

#include "../../Common.h"

template<typename T, int AS, int N>
class OrderNModel {
	uint32_t *cumul;
	uint32_t *freq;

	size_t seenSoFar;

	uint32_t sz;
	uint32_t pos;

	char history[N];
	char historyPos;

public:
	OrderNModel (void);
	~OrderNModel (void);

public:
	bool active (void) const;
	uint32_t getLow (T c) const;
	uint32_t getFreq (T c) const;
	uint32_t getSpan (void) const;
	void add (T c);
	T find (uint32_t cnt);
};

#include "OrderNModel.tcc"

#endif