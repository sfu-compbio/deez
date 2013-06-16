#ifndef PairedEnd_H
#define PairedEnd_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

struct PairedEndInfo {
	int chr;
	int pos;
	int tlen;

	PairedEndInfo (void):
		chr(0), pos(0), tlen(0) {}
	PairedEndInfo (int c, int p, int t):
		chr(c), pos(p), tlen(t) {}
};

class PairedEndCompressor: public GenericCompressor<PairedEndInfo, GzipCompressionStream<6> > {
	virtual const char *getID () const { return "PairedEnd"; }

public:
	PairedEndCompressor(int blockSize):
		GenericCompressor<PairedEndInfo, GzipCompressionStream<6> >(blockSize) {}
};

class PairedEndDecompressor: public GenericDecompressor<PairedEndInfo, GzipDecompressionStream> {
	virtual const char *getID () const { return "PairedEnd"; }

public:
	PairedEndDecompressor(int blockSize):
		GenericDecompressor<PairedEndInfo, GzipDecompressionStream>(blockSize) {}
};

#endif

