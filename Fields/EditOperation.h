#ifndef EditOperation_H
#define EditOperation_H

#include <string>

#include "../Common.h"
#include "../Streams/GzipStream.h"
#include "../Streams/Order0Stream.h"
#include "../Engines/GenericEngine.h"
#include "../Engines/StringEngine.h"

class SequenceCompressor;
class SequenceDecompressor;

struct EditOperation {
	size_t start;
	size_t end;
	std::string seq;
	std::string op;
	
	EditOperation() {}
	EditOperation(size_t s, const std::string &se, const std::string &o) :
		start(s), seq(se), op(o)
	{
		end = s;
		size_t size = 0;
		if (op != "*") for (size_t pos = 0; pos < op.length(); pos++) {
			if (isdigit(op[pos])) {
				size = size * 10 + (op[pos] - '0');
				continue;
			}
			switch (op[pos]) {
				case 'M':
				case '=':
				case 'X':
				case 'D':
				case 'N':
					end += size;
					break;
				default:
					break;
			}
			size = 0;
		}
	}
};

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

enum {
	OPCODES,
	SEQPOS,
	SEQEND,
	XLEN,
	HSLEN,
	LEN,
	OPLEN
};
const int OUTSIZE=7;

class EditOperationCompressor: 
	public GenericCompressor<EditOperation, GzipCompressionStream<6> >
{
	CompressionStream *unknownStream[OUTSIZE];
	CompressionStream *stitchStream;
	CompressionStream *locationStream;

	char *fixed;
	size_t fixedStart;

public:
	EditOperationCompressor(int blockSize);
	virtual ~EditOperationCompressor(void);

public:
	void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	void getIndexData (Array<uint8_t> &out);
	size_t compressedSize(void) { 
		LOGN("[Nucleotides %'lu Locations %'lu Stitch %'lu ", 
			stream->getCount(),
			locationStream->getCount(),
			stitchStream->getCount()
		);
		int res = 0;
		for (int i = 0; i < OUTSIZE; i++) {
			LOGN("OP%d %'lu ", i, unknownStream[i]->getCount());
			res += unknownStream[i]->getCount();
		}
		LOGN("]\n");
		return stream->getCount() + locationStream->getCount() + stitchStream->getCount() + res;
	}
	
private:
	friend class SequenceCompressor;
	void setFixed(char *f, size_t fs);
	const EditOperation &operator[] (int idx);
	
	void addOperation(char op, int seqPos, int size, Array<uint8_t> *out);
	void addEditOperation(const EditOperation &eo,
		ACTGStream &nucleotides, Array<uint8_t> *out);

};

class EditOperationDecompressor: 
	public GenericDecompressor<EditOperation, GzipDecompressionStream>  
{
	DecompressionStream *unknownStream;
	DecompressionStream *operandStream;
	DecompressionStream *lengthStream;

	DecompressionStream *stitchStream;
	DecompressionStream *locationStream;

	char *fixed;
	size_t fixedStart;

public:
	EditOperationDecompressor(int blockSize);
	virtual ~EditOperationDecompressor(void);

public:
	void importRecords (uint8_t *in, size_t in_size);
	void setIndexData (uint8_t *in, size_t in_size);

private:
	EditOperation getEditOperation (size_t loc, ACTGStream &nucleotides, uint8_t *&op, uint8_t *&len);

	friend class SequenceDecompressor;
	void setFixed(char *f, size_t fs);
};

#endif // EditOperation_H
