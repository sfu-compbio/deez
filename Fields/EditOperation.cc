#include "EditOperation.h"
using namespace std;

EditOperationCompressor::EditOperationCompressor (int blockSize):
	GenericCompressor<EditOperation, GzipCompressionStream<6> >(blockSize),
	fixed(0), fixedStart(0)
{
	operandStream = new GzipCompressionStream<6>();
	lengthStream = new GzipCompressionStream<6>();
	unknownStream  = new GzipCompressionStream<6>();
	locationStream = new AC0CompressionStream<256>();
	stitchStream = new GzipCompressionStream<6>();
}


EditOperationCompressor::~EditOperationCompressor (void) {
	delete operandStream;
	delete lengthStream;
	delete unknownStream;
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

void EditOperationCompressor::addOperation (char op, int seqPos, int size,
	Array<uint8_t> &operands, Array<uint8_t> &lengths) 
{
	//LOGN("\n <%c %d %d> ", op, seqPos, size);
	if (op == '=' || op == '*') 
		return;
	seqPos++; // Avoid zeros
	addEncoded(seqPos, lengths);
	if (!size) {
		operands.add('0');
		operands.add(op);
	}
	else if (op == 'N') {
		operands.add(op);
		assert(size > 0);
		addEncoded(size, lengths);
	}
	// seems to be working very well for XISD  [HP]
	else for (int i = 0; i < size; i++) 
		operands.add(op); 
	operands.add(0);
}

void EditOperationCompressor::addEditOperation(const EditOperation &eo,
	ACTGStream &nucleotides, Array<uint8_t> &operands, Array<uint8_t> &lengths) 
{
	if (eo.op == "*") {
		if (eo.seq != "*") {
			nucleotides.add(eo.seq.c_str(), eo.seq.size());
			addEncoded(eo.seq.size() + 1, lengths);
		}
		else 
			addEncoded(1, lengths);
		operands.add('*');
		operands.add(0);
		return;
	}
	/*else if (eo.seq == "*") { // weird, but happens!
		addEncoded(1, lengths);
		for (size_t pos = 0; pos < eo.op.size(); pos++) {
			if (isdigit(eo.op[pos])) {
				size = size * 10 + (eo.op[pos] - '0');
				continue;
			}
			operands.add('*');
			switch (eo.op[pos]) {
				addOperation(lastOP, 0, size, operands, lengths);
				break;
			}
			size = 0;
		}
		operands.add(0);
		addEncoded(eo.seq.size() + 1, lengths);
	}*/
	assert(fixed != 0);

	size_t size   = 0;
	size_t genPos = eo.start - fixedStart;
	size_t seqPos = 0;

	char lastOP = 0;
	int  lastOPSize = 0;

	bool checkSequence = eo.seq[0] != '*';

	for (size_t pos = 0; pos < eo.op.size(); pos++) {
		if (isdigit(eo.op[pos])) {
			size = size * 10 + (eo.op[pos] - '0');
			continue;
		}

		lastOP = 0;
		lastOPSize = 0;
		// M -> X=
		// =,X,I,S,D,N,H,P -> same
		switch (eo.op[pos]) {
			case 'M': // any
			case '=': // match
			case 'X': // mismatch
			//LOG("%c~%d %d\n",eo.op[pos], size,lastOP);
				if (!size) {
					//addOperation(lastOP, seqPos, lastOPSize, operands, lengths);
					lastOP = 'X' /*eo.op[pos]*/, lastOPSize = 0;
				}
				for (size_t i = 0; i < size; i++) {
					if (checkSequence && eo.seq[seqPos] == fixed[genPos]) {
						if (lastOP == '=')
							lastOPSize++;
						else {
							if (lastOP) addOperation(lastOP, seqPos, lastOPSize, operands, lengths);
							lastOP = '=', lastOPSize = 1;
						}
					}
					else if (checkSequence && lastOP == 'X')
						nucleotides.add(eo.seq.c_str() + seqPos, 1), lastOPSize++;
					else {
						if (lastOP) addOperation(lastOP, seqPos, lastOPSize, operands, lengths);
						if (checkSequence) nucleotides.add(eo.seq.c_str() + seqPos, 1);
						lastOP = 'X', lastOPSize = 1;
					}
					genPos++;
					seqPos++;
				}
				if (lastOP)
					addOperation(lastOP, seqPos, lastOPSize, operands, lengths);
				break;
			case 'I':
			case 'S':
				if (checkSequence) nucleotides.add(eo.seq.c_str() + seqPos, size);	
				seqPos += size;
				addOperation(eo.op[pos], seqPos, size, operands, lengths);
				break;
			case 'D':
			case 'N':
				addOperation(eo.op[pos], seqPos, size, operands, lengths);
				genPos += size;
				break;
			case 'H':
			case 'P':
				addOperation(eo.op[pos], seqPos, size, operands, lengths);
				break;
			default:
				throw DZException("Bad CIGAR detected: %s", eo.op.c_str());
		}
		size = 0;
	}
	operands.add(0);
	addEncoded((checkSequence ? eo.seq.size() : 0) + 1, lengths);
}

void EditOperationCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	Array<uint8_t> operands(k * records[0].op.size(), MB);
	Array<uint8_t> lengths(k * records[0].op.size(), MB);

	Array<uint8_t> locations(k);
	Array<uint32_t> stitches(k);	

	ACTGStream nucleotides(k * records[0].seq.size(), MB);
	nucleotides.initEncode();

	uint32_t lastLoc = 0;
	for (size_t i = 0; i < k; i++) {
		if (records[i].start - lastLoc >= 254)
			locations.add(255), stitches.add(records[i].start);
		else
			locations.add(records[i].start - lastLoc);
		lastLoc = records[i].start;

		addEditOperation(records[i], nucleotides, operands, lengths);
	}
	
	compressArray(stitchStream, stitches, out, out_offset);
	compressArray(locationStream, locations, out, out_offset);

	nucleotides.flush();
	compressArray(stream, nucleotides.seqvec, out, out_offset);
	compressArray(stream, nucleotides.Nvec, out, out_offset);

	compressArray(operandStream, operands, out, out_offset);
	compressArray(lengthStream, lengths, out, out_offset);

	records.remove_first_n(k);
}

/****************************************************************************************************/


EditOperationDecompressor::EditOperationDecompressor (int blockSize):
	GenericDecompressor<EditOperation, GzipDecompressionStream>(blockSize)
{
	unknownStream  = new GzipDecompressionStream();
	operandStream  = new GzipDecompressionStream();
	lengthStream   = new GzipDecompressionStream();
	locationStream = new AC0DecompressionStream<256>();
	stitchStream   = new GzipDecompressionStream();
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
	//assert(recordCount == records.size());

	Array<uint8_t> stitches;
	decompressArray(stitchStream, in, stitches);
	Array<uint8_t> locations;
	size_t sz = decompressArray(locationStream, in, locations);

	ACTGStream nucleotides;
	decompressArray(stream, in, nucleotides.seqvec);
	decompressArray(stream, in, nucleotides.Nvec);
	nucleotides.initDecode();

	Array<uint8_t> operands;
	decompressArray(operandStream, in, operands);
	Array<uint8_t> lengths;
	decompressArray(lengthStream, in, lengths);

	records.resize(0);

	size_t stitchIdx = 0;
	uint32_t lastLoc = 0;
	uint8_t *op = operands.data();
	uint8_t *len = lengths.data();
	for (size_t i = 0; i < sz; i++) {
		if (locations.data()[i] == 255)
			lastLoc = ((uint32_t*)stitches.data())[stitchIdx++];
		else
			lastLoc += locations.data()[i];
		records.add(getEditOperation(lastLoc, nucleotides, op, len));
	}

	recordCount = 0;
}

EditOperation EditOperationDecompressor::getEditOperation (size_t loc, ACTGStream &nucleotides, uint8_t *&op, uint8_t *&len) {
//	assert(fixed != NULL);
//	assert(loc >= fixedStart);

	EditOperation eo;
	eo.start = loc;
	eo.end = loc; 

	static Array<char> opChr(100, 100);
	static Array<int>  opLen(100, 100);

	opChr.resize(0);
	opLen.resize(0);

	int prevloc = 0;
	while (1) {
		// maybe gzip can catch this?
		size_t endPos = getEncoded(len);
		endPos--; // Avoid zeros fix
		
		//LOG(">> %c_%d %d %d", *op, *op, endPos, prevloc);

		// End case. Check is prevloc at end. If not, add = and exit
		if (!*op) { 
			if (!endPos)
				eo.seq = "*";
			else if (endPos > prevloc)  
				opChr.add('='), opLen.add(endPos - prevloc);
			op++;
			break;
		}
		// Unmapped case. Just add * and exit
		else if (*op == '*') {
			opChr.add('*'), opLen.add(endPos);
			op++; op++; 
			break;
		}
		// Other cases
		else {
			// Get op
			char c = *op;
			// Get length
			int l = 0;
			if (*op == '0') 
				c = *(++op), l = 0, ++op;
			else if (*op == 'N')
				l = getEncoded(len), op++;
			else while (*op == c)
				l++, op++; 
			op++; // 0

			// Do  we have trailing = ?
			if ((c == 'N' || c == 'D' || c == 'H' || c == 'P') && endPos > prevloc) 
				opChr.add('='), opLen.add(endPos - prevloc);
			else if (c != 'N' && c != 'D' && c != 'H' && c != 'P' && (int)endPos - l > prevloc) 
				opChr.add('='), opLen.add(endPos - l - prevloc);
			prevloc = endPos;
			opChr.add(c), opLen.add(l);
		} 
	}

	//for (int i = 0; i < opChr.size(); i++) 
	//	LOGN("%d%c ", opLen[i], opChr[i]);
	//LOG("");
 
	size_t genPos = loc - fixedStart;
	char lastOP = 0;
	int  lastOPSize = 0;
	for (int i = 0; i < opChr.size(); i++) {
		// restore original part
		switch (opChr[i]) {
			case '=':
				if (lastOP == 'M')
					lastOPSize += opLen[i];
				else 
					lastOP = 'M', lastOPSize = opLen[i];
				eo.seq.append(fixed + genPos, opLen[i]);
				genPos += opLen[i];
				break;

			case 'X':
				if (lastOP == 'M')
					lastOPSize += opLen[i];
				else 
					lastOP = 'M', lastOPSize = opLen[i];
				nucleotides.get(eo.seq, opLen[i]);
				genPos += opLen[i];
				break;

			case '*':
			case 'S':
			case 'I':
				nucleotides.get(eo.seq, opLen[i]);
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				if (opChr[i] == '*')
					eo.op = "*";
				else {
					eo.op += inttostr(opLen[i]) + char(opChr[i]);
					if (opChr[i] != 'S' && opChr[i] != 'I')
						eo.end += lastOPSize;
				}
				break;

			case 'H':
			case 'P':
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				eo.op += inttostr(opLen[i]) + char(opChr[i]);
			//	genPos += opLen[i];
				break;

			case 'D':
			case 'N': 
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				eo.op += inttostr(opLen[i]) + char(opChr[i]);
				genPos += opLen[i];
				break;
		}
	}
	if (lastOP != 0) {
		eo.op += inttostr(lastOPSize) + lastOP;
		if (lastOP != 'S' && lastOP != 'I')
			eo.end += lastOPSize;
	}

	if (eo.seq == "" || eo.seq[0] == '*')
		eo.seq = "*";
	return eo;
}

void EditOperationDecompressor::setIndexData (uint8_t *in, size_t in_size) {
	locationStream->setCurrentState(in, in_size);
}
