#include "OrderNModel.h"

template<typename T, int AS, int N>
OrderNModel<T, AS, N>::OrderNModel (void):
	seenSoFar(0), historyPos(0), pos(0), sz(1)
{
	for (int i = 0; i < N; i++)
		sz *= AS;
	LOG("sz = %'lld", 4*(sz+sz*AS));
	cumul = new uint32_t[sz];
	freq = new uint32_t[sz * AS];

	for (size_t i = 0; i < sz * AS; i++)
		freq[i] = 1;
	for (size_t i = 0; i < sz; i++)
		cumul[i] = AS;
}

template<typename T, int AS, int N>
OrderNModel<T, AS, N>::~OrderNModel (void) {
	delete[] cumul;
	delete[] freq;
}

template<typename T, int AS, int N>
bool OrderNModel<T, AS, N>::active (void) const {
	return seenSoFar >= N;
}

template<typename T, int AS, int N>
uint32_t OrderNModel<T, AS, N>::getLow (T c) const {
	uint32_t lo = 0;
	for (int i = 0; i < AS; i++)
		lo += freq[pos * AS + i];
	return lo;
}

template<typename T, int AS, int N>
uint32_t OrderNModel<T, AS, N>::getFreq (T c) const {
	return freq[pos * AS + c];
}

template<typename T, int AS, int N>
uint32_t OrderNModel<T, AS, N>::getSpan (void) const {
	return cumul[pos];
}

template<typename T, int AS, int N>
void OrderNModel<T, AS, N>::add (T c) {
	freq[pos * AS + c]++;
	cumul[pos]++;

	if (cumul[pos] == -1) {
		throw "not implemented";
	}

	uint32_t xp = pos;
	if (active()) pos -= history[historyPos] * (sz / AS);
	pos *= AS;
	pos += c;
	//LOG("pos: %u -> %u ~ %d < %d\n",xp,pos,c,history[historyPos]);

	history[historyPos] = c;
	historyPos = (historyPos + 1) % N;

	seenSoFar++;
}

template<typename T, int AS, int N>
T OrderNModel<T, AS, N>::find (uint32_t cnt) {
	uint32_t hi = 0;
	for (size_t i = 0; i < AS; i++) {
		hi += freq[pos * AS + i];
		if (hi > cnt) return i;
	}
	throw "AC find failed";
}
