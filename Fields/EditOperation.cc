#include "EditOperation.h"
using namespace std;

EditOperationCompressor::EditOperationCompressor (int blockSize):
	GenericCompressor<EditOperation, GzipCompressionStream<6> >(blockSize),
	fixed(0), fixedStart(0),
	sequence(0), sequencePos(0),
	Nfixes(0), NfixesPos(0)
{
	unknownStream = new GzipCompressionStream<6>();
	operandStream = new GzipCompressionStream<6>();
	lengthStream = new GzipCompressionStream<6>();
	
	locationStream = new AC0CompressionStream<256>();
	stitchStream = new GzipCompressionStream<6>();
}

EditOperationCompressor::~EditOperationCompressor (void) {
	delete unknownStream;
	delete operandStream;
	delete lengthStream;
	delete locationStream;
	delete stitchStream;
}

const EditOperation& EditOperationCompressor::operator[] (int idx) {
	assert(idx < records.size());
	return records[idx];
}

void EditOperationCompressor::getIndexData (Array<uint8_t> &out) {
	out.resize(0);
	locationStream->getCurrentState(out);
}

void EditOperationCompressor::setFixed (char *f, size_t fs) {
	fixed = f, fixedStart = fs;
}

void EditOperationCompressor::addOperation (char op, int size,
	Array<uint8_t> &operands, Array<uint8_t> &lengths) 
{
	operands.add(op);
	assert(size > 0);
	//LOG("$EOL %d", size);
	if (size < (1 << 8))
		lengths.add(size);
	else if (size < (1 << 16)) {
		lengths.add(0);
		lengths.add((size >> 8) & 0xff);
		lengths.add(size & 0xff);
	}
	else {
		REPEAT(2) lengths.add(0);
		lengths.add((size >> 24) & 0xff);
		lengths.add((size >> 16) & 0xff);
		lengths.add((size >> 8) & 0xff);
		lengths.add(size & 0xff);	
	}
}

void EditOperationCompressor::addSequence (const char *seq, size_t len, 
	Array<uint8_t> &nucleotides, Array<uint8_t> &unknowns) 
{
	for (size_t i = 0; i < len; i++) {
		assert(isupper(seq[i]));

		if (sequencePos == 4) 
			nucleotides.add(sequence), sequencePos = 0;
		if (NfixesPos == 8)
			unknowns.add(Nfixes), NfixesPos = 0;
		
		if (seq[i] == 'N') {
			sequence <<= 2, sequencePos++;
			Nfixes <<= 1, Nfixes |= 1, NfixesPos++;
			continue;
		}
		if (seq[i] == 'A')
			Nfixes <<= 1, NfixesPos++;
		sequence <<= 2, sequence |= "\0\0\001\0\0\0\002\0\0\0\0\0\0\0\0\0\0\0\0\003"[seq[i] - 'A'];
		sequencePos++;
	}
}

void EditOperationCompressor::addEditOperation(const EditOperation &eo,
	Array<uint8_t> &nucleotides, Array<uint8_t> &unknowns, 
	Array<uint8_t> &operands, Array<uint8_t> &lengths) 
{
	if (eo.op == "*") {
		addOperation('*', eo.seq.size(), operands, lengths);
		addSequence(eo.seq.c_str(), eo.seq.size(), nucleotides, unknowns);
		operands.add(0);
		return;
	}
	assert(fixed != 0);

	size_t size   = 0;
	size_t genPos = eo.start - fixedStart;
	size_t seqPos = 0;

	char lastOP = 0;
	int  lastOPSize = 0;

	for (size_t pos = 0; pos < eo.op.size(); pos++) {
		if (isdigit(eo.op[pos])) {
			size = size * 10 + (eo.op[pos] - '0');
			continue;
		}

		lastOP = 0;
		lastOPSize = 0;
		// M -> X=
		// =,X,I,S,D,N -> same
		// H,P -> not supported
		switch (eo.op[pos]) {
			case 'M': // any
			case '=': // match
			case 'X': // mismatch
				for (size_t i = 0; i < size; i++) {
					if (eo.seq[seqPos] == fixed[genPos]) {
						if (lastOP == '=')
							lastOPSize++;
						else {
							if (lastOP) addOperation(lastOP, lastOPSize, operands, lengths);
							lastOP = '=', lastOPSize = 1;
						}
					}
					else if (lastOP == 'X')
						addSequence(eo.seq.c_str() + seqPos, 1, nucleotides, unknowns), lastOPSize++;
					else {
						if (lastOP) addOperation(lastOP, lastOPSize, operands, lengths);
						addSequence(eo.seq.c_str() + seqPos, 1, nucleotides, unknowns);
						lastOP = 'X', lastOPSize = 1;
					}
					genPos++;
					seqPos++;
				}
				if (lastOP)
					addOperation(lastOP, lastOPSize, operands, lengths);
				break;
			case 'I':
			case 'S':
				addOperation(eo.op[pos], size, operands, lengths);
				addSequence(eo.seq.c_str() + seqPos, size, nucleotides, unknowns);
				seqPos += size;
				break;
			case 'D':
			case 'N':
				addOperation(eo.op[pos], size, operands, lengths);
				genPos += size;
				break;
			case 'H':
			case 'P':
				throw DZException("Not Supported");
			default:
				throw DZException("Bad CIGAR detected: %s", eo.op.c_str());
		}
		size = 0;
	}
	operands.add(0);
}

void EditOperationCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	Array<uint8_t> nucleotides(k * records[0].seq.size(), MB);
	Array<uint8_t> unknowns(k * records[0].seq.size(), MB);
	Array<uint8_t> operands(k * records[0].op.size(), MB);
	Array<uint8_t> lengths(k * records[0].op.size(), MB);

	Array<uint8_t> locations(k);
	Array<uint32_t> stitches(k);	

	sequence = 0, sequencePos = 0;
	Nfixes = 0, NfixesPos = 0;
	uint32_t lastLoc = 0;
	for (size_t i = 0; i < k; i++) {
		if (records[i].start - lastLoc >= 254)
			locations.add(255), stitches.add(records[i].start);
		else
			locations.add(records[i].start - lastLoc);
		lastLoc = records[i].start;

		addEditOperation(records[i], nucleotides, unknowns, operands, lengths);
	}
	while (sequencePos != 4) sequence <<= 2, sequencePos++;
	nucleotides.add(sequence);
	while (NfixesPos != 8) Nfixes <<= 1, NfixesPos++;
	unknowns.add(Nfixes);

	compressArray(stitchStream, stitches, out, out_offset);
	compressArray(locationStream, locations, out, out_offset);

	compressArray(stream, nucleotides, out, out_offset);
	compressArray(unknownStream, unknowns, out, out_offset);
	compressArray(operandStream, operands, out, out_offset);
	compressArray(lengthStream, lengths, out, out_offset);

	records.remove_first_n(k);
}

/****************************************************************************************************/


EditOperationDecompressor::EditOperationDecompressor (int blockSize):
	GenericDecompressor<EditOperation, GzipDecompressionStream>(blockSize),
	sequence(0), sequencePos(0),
	Nfixes(0), NfixesPos(0)
{
	unknownStream = new GzipDecompressionStream();
	operandStream = new GzipDecompressionStream();
	lengthStream = new GzipDecompressionStream();
	locationStream = new AC0DecompressionStream<256>();
	stitchStream = new GzipDecompressionStream();
}

EditOperationDecompressor::~EditOperationDecompressor (void) {
	delete unknownStream;
	delete operandStream;
	delete lengthStream;
	delete locationStream;
	delete stitchStream;
}

void EditOperationDecompressor::setFixed (char *f, size_t fs) {
	fixed = f, fixedStart = fs;
}

void EditOperationDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) return;
	assert(recordCount == records.size());

	Array<uint8_t> stitches;
	decompressArray(stitchStream, in, stitches);
	Array<uint8_t> locations;
	size_t sz = decompressArray(locationStream, in, locations);

	Array<uint8_t> nucleotides;
	decompressArray(stream, in, nucleotides);
	Array<uint8_t> unknowns;
	decompressArray(unknownStream, in, unknowns);
	Array<uint8_t> operands;
	decompressArray(operandStream, in, operands);
	Array<uint8_t> lengths;
	decompressArray(lengthStream, in, lengths);

	records.resize(0);

	sequence = nucleotides.data(), sequencePos = 6;
	Nfixes = unknowns.data(), NfixesPos = 7;

	size_t stitchIdx = 0;
	uint32_t lastLoc = 0;
	uint8_t *op = operands.data();
	uint8_t *len = lengths.data();
	for (size_t i = 0; i < sz; i++) {
		if (locations.data()[i] == 255)
			lastLoc = stitchIdx++[(uint32_t*)stitches.data()];
		else
			lastLoc += locations.data()[i];
		records.add(getEditOperation(lastLoc, op, len));
	}

	recordCount = 0;
}

void EditOperationDecompressor::getSequence(string &out, size_t sz) {
	char DNA[] = "ACGT";
	char AN[] = "AN";

	for (int i = 0; i < sz; i++) {
		char c = (*sequence >> sequencePos) & 3;
		if (!c) {
			out += AN[(*Nfixes >> NfixesPos) & 1];
			if (NfixesPos == 0) NfixesPos = 7, Nfixes++;
			else NfixesPos--;
		}
		else out += DNA[c];
		
		if (sequencePos == 0) sequencePos = 6, sequence++;
		else sequencePos -= 2;
	}
}

EditOperation EditOperationDecompressor::getEditOperation (size_t loc, uint8_t *&op, uint8_t *&len) {
	assert(fixed != NULL);
	assert(loc >= fixedStart);

	size_t genPos = loc - fixedStart;
	char lastOP = 0;
	int  lastOPSize = 0;

	EditOperation eo;
	eo.start = loc;
	eo.end = loc; 

	//static int xx(1);
	//LOG("\n%d.....",xx++);
	while (*op) {
		// maybe gzip can catch this?
		int T = 1;
		if (!*len) T = 2, len++;
		if (!*len) T = 4, len++;
		size_t size = 0;
		REPEAT(T) size |= *len++ << (8 * (T - _ - 1));
		assert(size > 0);
	//	LOGN(" %lu %d ", genPos, size);
		
		switch (*op) {
			case '=':
				if (lastOP == 'M')
					lastOPSize += size;
				else 
					lastOP = 'M', lastOPSize = size;
				eo.seq.append(fixed + genPos, size);
				genPos += size;
				break;

			case 'X':
				if (lastOP == 'M')
					lastOPSize += size;
				else 
					lastOP = 'M', lastOPSize = size;
				getSequence(eo.seq, size);
				genPos += size;
				break;

			case '*':
			case 'S':
			case 'I':
				getSequence(eo.seq, size);
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				if (*op == '*')
					eo.op = "*";
				else {
					eo.op += inttostr(size) + char(*op);
					if (*op != 'S' && *op != 'I')
						eo.end += lastOPSize;
				}
				break;

			case 'D':
			case 'N': 
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				eo.op += inttostr(size) + char(*op);
				genPos += size;
				break;
		}
		op++;
	}
	if (lastOP != 0) {
		eo.op += inttostr(lastOPSize) + lastOP;
		if (lastOP != 'S' && lastOP != 'I')
			eo.end += lastOPSize;
	}
	op++;

	return eo;
}

void EditOperationDecompressor::setIndexData (uint8_t *in, size_t in_size) {
	locationStream->setCurrentState(in, in_size);
}
