#ifndef ACGTStream_H
#define ACGTStream_H

#include "../Common.h"

struct ACTGStream {
	Array<uint8_t> seqvec, Nvec;
	int seqcnt, Ncnt;
	uint8_t seq, N;
	uint8_t *pseq, *pN;

	ACTGStream() {}
	ACTGStream(size_t cap, size_t ext):
		seqvec(cap, ext), Nvec(cap, ext) {}

	void initEncode (void) {
		seqcnt = 0, Ncnt = 0, seq = 0, N = 0;
	}

	void initDecode (void) {
		seqcnt = 6, pseq = seqvec.data();
		Ncnt = 7, pN = Nvec.data();
	}

	void add_(char c) {
		if (Ncnt == 8)
			Nvec.add(N), Ncnt = 0;
		if (c == 'N') {
			seqvec.add(0);
			N <<= 1, N |= 1, Ncnt++;
			return;
		}
		c = "\0\0\001\0\0\0\002\0\0\0\0\0\0\0\0\0\0\0\0\003"[c - 'A'];
		seqvec.add(c);
	}

	void add (char c) {
		assert(isupper(c));
		if (seqcnt == 4) 
			seqvec.add(seq), seqcnt = 0;
		if (Ncnt == 8)
			Nvec.add(N), Ncnt = 0;
		if (c == 'N') {
			seq <<= 2, seqcnt++;
			N <<= 1, N |= 1, Ncnt++;
			return;
		}
		if (c == 'A')
			N <<= 1, Ncnt++;
		seq <<= 2;
		seq |= "\0\0\001\0\0\0\002\0\0\0\0\0\0\0\0\0\0\0\0\003"[c - 'A'];
		seqcnt++;
	}

	void add (const char *str, size_t len)  {
		for (size_t i = 0; i < len; i++) 
			add(str[i]);
	}

	void flush (void) {
		while (seqcnt != 4) seq <<= 2, seqcnt++;
		seqvec.add(seq), seqcnt = 0;
		while (Ncnt != 8) N <<= 1, Ncnt++;
		Nvec.add(N), Ncnt = 0;
	}

	void get (string &out, size_t sz) {
		const char *DNA = "ACGT";
		const char *AN  = "AN";

		for (int i = 0; i < sz; i++) {
			char c = (*pseq >> seqcnt) & 3;
			if (!c) {
				out += AN[(*pN >> Ncnt) & 1];
				if (Ncnt == 0) Ncnt = 7, pN++;
				else Ncnt--;
			}
			else out += DNA[c];
			
			if (seqcnt == 0) seqcnt = 6, pseq++;
			else seqcnt -= 2;
		}
	}
};

#endif // ACGTStream_H