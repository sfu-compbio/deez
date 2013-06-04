#ifndef PairedEnd_H
#define PairedEnd_H

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Engines/GenericEngine.h"

struct PairedEndInfo {
	int chr;
	int pos;
	int tlen;

	PairedEndInfo (void) {}
	PairedEndInfo (int c, int p, int t):
		chr(c), pos(p), tlen(t) {}
};

typedef GenericCompressor<PairedEndInfo, GzipCompressionStream<6> >	PairedEndCompressor;
typedef GenericDecompressor<PairedEndInfo, GzipDecompressionStream>	PairedEndDecompressor;

#endif

