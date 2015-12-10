#include "EditOperation.h"
using namespace std;

enum EditOperationFields {
	OPCODES,
	SEQPOS,
	SEQEND,
	XLEN,
	HSLEN,
	LEN,
	OPLEN,
	ENUM_COUNT
};

EditOperationCompressor::EditOperationCompressor (int blockSize):
	GenericCompressor<EditOperation, GzipCompressionStream<6> >(blockSize),
	fixed(0), fixedStart(0)
{
	locationStream = new AC0CompressionStream<256>();
	stitchStream = new GzipCompressionStream<6>();

	for (int i = 0; i < EditOperationFields::ENUM_COUNT; i++)
		streams.push_back(new GzipCompressionStream<6>());
}

EditOperationCompressor::~EditOperationCompressor (void) 
{
	for (int i = 0; i < streams.size(); i++)
		delete streams[i];
	delete locationStream;
	delete stitchStream;
}

size_t EditOperationCompressor::compressedSize(void) 
{ 
	int res = 0;
	for (int i = 0; i < streams.size(); i++) 
		res += streams[i]->getCount();
	return stream->getCount() + locationStream->getCount() + stitchStream->getCount() + res;
}

void EditOperationCompressor::printDetails(void) 
{
	LOG("  Nuc       : %'20lu", stream->getCount());
	LOG("  Positions : %'20lu", locationStream->getCount());
	LOG("  Stitches  : %'20lu", stitchStream->getCount());
	vector<const char*> s { "OPCODES", "SEQPOS", "SEQEND", "XLEN", "HSLEN", "LEN", "OPLEN" };
	for (int i = 0; i < streams.size(); i++) 
		LOG("  %-10s: %'20lu ", s[i], streams[i]->getCount());
}

const EditOperation& EditOperationCompressor::operator[] (int idx) 
{
	assert(idx < records.size());
	return records[idx];
}

void EditOperationCompressor::getIndexData (Array<uint8_t> &out) 
{
	out.resize(0);
	locationStream->getCurrentState(out);
}

void EditOperationCompressor::setFixed (char *f, size_t fs) 
{
	fixed = f, fixedStart = fs;
}

void EditOperationCompressor::addOperation (char op, int seqPos, int size, vector<Array<uint8_t>> &out) 
{
	if (op == '=' || op == '*') 
		return;

	seqPos++; // Avoid zeros
	addEncoded(seqPos, out[EditOperationFields::SEQPOS]);
	if (!size) {
		out[EditOperationFields::OPCODES].add('0');
		out[EditOperationFields::OPCODES].add(op);
	} else if (op == 'X') {
		out[EditOperationFields::OPCODES].add(op); 
		addEncoded(size, out[EditOperationFields::XLEN]);
	} else if (op == 'H' || op == 'S') {
		out[EditOperationFields::OPCODES].add(op); 
		addEncoded(size, out[EditOperationFields::HSLEN]);
	} else {
		out[EditOperationFields::OPCODES].add(op); 
		addEncoded(size, out[EditOperationFields::LEN]);
	}
}

void EditOperationCompressor::addEditOperation(const EditOperation &eo, ACTGStream &nucleotides, vector<Array<uint8_t>> &out) 
{
	if (eo.op == "*") {
		if (eo.seq != "*") {
			nucleotides.add(eo.seq.c_str(), eo.seq.size());
			addEncoded(eo.seq.size() + 1, out[EditOperationFields::SEQEND]);
		} else {
			addEncoded(1, out[EditOperationFields::SEQEND]);
		}
		out[EditOperationFields::OPCODES].add('*');
		addEncoded(1 + 1, out[EditOperationFields::OPLEN]);
		return;
	}
	assert(fixed != 0);

	size_t size   = 0;
	size_t genPos = eo.start - fixedStart;
	size_t seqPos = 0, prevSeqPos = 0;

	char lastOP = 0;
	int  lastOPSize = 0;

	bool checkSequence = eo.seq[0] != '*';
	int opcodeOffset = out[EditOperationFields::OPCODES].size();
	for (size_t pos = 0; pos < eo.op.size(); pos++) {
		if (isdigit(eo.op[pos])) {
			size = size * 10 + (eo.op[pos] - '0');
			continue;
		}

		lastOP = 0;
		lastOPSize = 0;
		switch (eo.op[pos]) {
			case 'M': // any; will become =X in DeeZ's internal structure
			case '=': // match
			case 'X': // mismatch
				if (!size) {
					lastOP = 'X', lastOPSize = 0;
				}
				for (size_t i = 0; i < size; i++) {
					if (checkSequence && eo.seq[seqPos] == fixed[genPos]) {
						if (lastOP == '=')
							lastOPSize++;
						else {
							if (lastOP && lastOP != '=') {
								addOperation(lastOP, seqPos - prevSeqPos, lastOPSize, out), prevSeqPos = seqPos;
							}
							lastOP = '=', lastOPSize = 1;
						}
					}
					else if (checkSequence && lastOP == 'X')
						nucleotides.add(eo.seq.c_str() + seqPos, 1), lastOPSize++;
					else {
						if (lastOP && lastOP != '=') {
							addOperation(lastOP, seqPos - prevSeqPos, lastOPSize, out), prevSeqPos = seqPos;
						}
						if (checkSequence) nucleotides.add(eo.seq.c_str() + seqPos, 1);
						lastOP = 'X', lastOPSize = 1;
					}
					genPos++;
					seqPos++;
				}
				if (lastOP && lastOP != '=')
					addOperation(lastOP, seqPos - prevSeqPos, lastOPSize, out), prevSeqPos = seqPos;
				break;
			case 'I':
			case 'S':
				if (checkSequence) nucleotides.add(eo.seq.c_str() + seqPos, size);	
				seqPos += size;
				addOperation(eo.op[pos], seqPos - prevSeqPos, size, out), prevSeqPos = seqPos;
				break;
			case 'D':
			case 'N':
				addOperation(eo.op[pos], seqPos - prevSeqPos, size, out), prevSeqPos = seqPos;
				genPos += size;
				break;
			case 'H':
			case 'P':
				addOperation(eo.op[pos], seqPos - prevSeqPos, size, out), prevSeqPos = seqPos;
				break;
			default:
				throw DZException("Bad CIGAR detected: %s", eo.op.c_str());
		}
		size = 0;
	}
	addEncoded(out[EditOperationFields::OPCODES].size() - opcodeOffset + 1, out[EditOperationFields::OPLEN]);
	addEncoded((checkSequence ? eo.seq.size() : 0) + 1, out[EditOperationFields::SEQEND]);
}

void EditOperationCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	vector<Array<uint8_t>> oa;
	for (int i = 0; i < streams.size(); i++) 
		oa.push_back(Array<uint8_t>(k, MB));
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

		addEditOperation(records[i], nucleotides, oa);
	}
	
	compressArray(stitchStream, stitches, out, out_offset);
	compressArray(locationStream, locations, out, out_offset);

	nucleotides.flush();
	compressArray(stream, nucleotides.seqvec, out, out_offset);
	compressArray(stream, nucleotides.Nvec, out, out_offset);

	for (int i = 0; i < streams.size(); i++)
		compressArray(streams[i], oa[i], out, out_offset);
	
	records.remove_first_n(k);
}

/****************************************************************************************************/


EditOperationDecompressor::EditOperationDecompressor (int blockSize):
	GenericDecompressor<EditOperation, GzipDecompressionStream>(blockSize)
{
	for (int i = 0; i < EditOperationFields::ENUM_COUNT; i++)
		streams.push_back(new GzipDecompressionStream());
	locationStream = new AC0DecompressionStream<256>();
	stitchStream   = new GzipDecompressionStream();
}

EditOperationDecompressor::~EditOperationDecompressor (void) 
{
	for (int i = 0; i < streams.size(); i++)
		delete streams[i];
	delete locationStream;
	delete stitchStream;
}

void EditOperationDecompressor::setFixed (char *f, size_t fs) 
{
	fixed = f, fixedStart = fs;
}

void EditOperationDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	Array<uint8_t> stitches;
	decompressArray(stitchStream, in, stitches);
	Array<uint8_t> locations;
	size_t sz = decompressArray(locationStream, in, locations);


	ACTGStream nucleotides;
	decompressArray(stream, in, nucleotides.seqvec);
	decompressArray(stream, in, nucleotides.Nvec);
	nucleotides.initDecode();

	vector<Array<uint8_t>> oa(streams.size());
	vector<uint8_t*> fields;
	for (int i = 0; i < streams.size(); i++)  {
		decompressArray(streams[i], in, oa[i]);
		fields.push_back(oa[i].data());
	}

	records.resize(0);

	size_t stitchIdx = 0;
	uint32_t lastLoc = 0;
	for (size_t i = 0; i < sz; i++) {
		if (locations.data()[i] == 255)
			lastLoc = ((uint32_t*)stitches.data())[stitchIdx++];
		else
			lastLoc += locations.data()[i];
		records.add(getEditOperation(lastLoc, nucleotides, fields));
	}

	recordCount = 0;
}

EditOperation EditOperationDecompressor::getEditOperation (size_t loc, ACTGStream &nucleotides, vector<uint8_t*> &fields) 
{
	EditOperation eo;
	eo.start = eo.end = loc;
	
	static Array<char> opChr(100, 100);
	static Array<int>  opLen(100, 100);

	opChr.resize(0);
	opLen.resize(0);

	size_t prevLoc = 0, endPos = 0; 
	int count = getEncoded(fields[EditOperationFields::OPLEN]) - 1;
	for (int i = 0; i < count; i++) {
		char c = *fields[EditOperationFields::OPCODES]++;
		assert(c);
		int l = 0;
		if (c == '*') { // Unmapped case. Just add * and exit
			assert(count == 1);
			endPos = getEncoded(fields[EditOperationFields::SEQEND]) - 1;
			opChr.add('*'), opLen.add(endPos);
			goto end;
		} 
		
		endPos += getEncoded(fields[EditOperationFields::SEQPOS]) - 1;
		if (c == '0') {
			c = *fields[EditOperationFields::OPCODES]++, l = 0; i++;
		} else if (c == 'X') {
			l = getEncoded(fields[EditOperationFields::XLEN]);
		} else if (c == 'H' || c == 'S') {
			l = getEncoded(fields[EditOperationFields::HSLEN]);
		} else {
			l = getEncoded(fields[EditOperationFields::LEN]);
		}

		// Do  we have trailing = ?
		if ((c == 'N' || c == 'D' || c == 'H' || c == 'P') && endPos > prevLoc) 
			opChr.add('='), opLen.add(endPos - prevLoc);
		else if (c != 'N' && c != 'D' && c != 'H' && c != 'P' && endPos - l > prevLoc) 
			opChr.add('='), opLen.add(endPos - l - prevLoc);
		prevLoc = endPos;
		opChr.add(c), opLen.add(l);
	}
	// End case. Check is prevLoc at end. If not, add = and exit
	endPos = getEncoded(fields[EditOperationFields::SEQEND]) - 1;
	if (!endPos)
		eo.seq = "*";
	else if (endPos > prevLoc)  
		opChr.add('='), opLen.add(endPos - prevLoc);

end:
	//for (int i = 0; i < opLen.size(); i++)
	//	LOG("%c %d", opChr[i], opLen[i]);

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
			case 'H':
			case 'P':
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				if (opChr[i] == '*') {
					eo.op = "*";
				} else {
					eo.op += inttostr(opLen[i]) + char(opChr[i]);
				}
				break;

			case 'D':
			case 'N': 
				if (lastOP != 0) {
					eo.op += inttostr(lastOPSize) + lastOP;
					if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
						eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				eo.op += inttostr(opLen[i]) + char(opChr[i]);
				eo.end += opLen[i];
				genPos += opLen[i];
				break;
		}
	}
	if (lastOP != 0) {
		eo.op += inttostr(lastOPSize) + lastOP;
		if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
			eo.end += lastOPSize;
	}

	if (eo.seq == "" || eo.seq[0] == '*')
		eo.seq = "*";
	return eo;
}

void EditOperationDecompressor::setIndexData (uint8_t *in, size_t in_size) 
{
	locationStream->setCurrentState(in, in_size);
}
