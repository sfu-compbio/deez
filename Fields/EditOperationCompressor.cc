#include "EditOperation.h"
using namespace std;

EditOperationCompressor::EditOperationCompressor (int blockSize):
	GenericCompressor<EditOperation, GzipCompressionStream<6> >(blockSize),
	fixed(0), fixedStart(0)
{
	locationStream = new AC0CompressionStream<rANS, 256>();
	stitchStream = new GzipCompressionStream<6>();

	for (int i = 0; i < Fields::ENUM_COUNT; i++)
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
	LOG("  Positions : %'20lu", locationStream->getCount());
	LOG("  Stitches  : %'20lu", stitchStream->getCount());
	vector<const char*> s { "Opcode", "SeqPos", "SeqLen", "XLen", "HSLen", "AnyLen", "OPLen", "ACGT", "ACGT+N" };
	for (int i = 0; i < streams.size(); i++) 
		LOG("  %-10s: %'20lu ", s[i], streams[i]->getCount());
}

void EditOperationCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	ZAMAN_START(Compress_EditOperation);

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

	ZAMAN_START(Compress_EditOperation_ACGT);
	nucleotides.flush();
	compressArray(streams[Fields::ACGT], nucleotides.seqvec, out, out_offset);
	compressArray(streams[Fields::ACGT], nucleotides.Nvec, out, out_offset);
	ZAMAN_END(Compress_EditOperation_ACGT);

	for (int i = 0; i < Fields::ACGT; i++)
		compressArray(streams[i], oa[i], out, out_offset);

	ZAMAN_END(Compress_EditOperation);
}

void EditOperationCompressor::addEditOperation(const EditOperation &eo, ACTGStream &nucleotides, vector<Array<uint8_t>> &out) 
{
	ZAMAN_START(Compress_EditOperation_AddEditOperation);
	
	if (eo.op == "*") {
		if (eo.seq != "*") {
			nucleotides.add(eo.seq.c_str(), eo.seq.size());
			addEncoded(eo.seq.size() + 1, out[Fields::SEQEND]);
		} else {
			addEncoded(1, out[Fields::SEQEND]);
		}
		out[Fields::OPCODES].add('*');
		addEncoded(1 + 1, out[Fields::OPLEN]);

		ZAMAN_END(Compress_EditOperation_AddEditOperation);
		return;
	}
	assert(fixed != 0);

	size_t size   = 0;
	size_t genPos = eo.start - fixedStart;
	size_t seqPos = 0, prevSeqPos = 0;

	char lastOP = 0;
	int  lastOPSize = 0;

	bool checkSequence = eo.seq[0] != '*';
	int opcodeOffset = out[Fields::OPCODES].size();
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
					} else if (checkSequence && lastOP == 'X') {
						nucleotides.add(eo.seq.c_str() + seqPos, 1), lastOPSize++;
					} else {
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
	addEncoded(out[Fields::OPCODES].size() - opcodeOffset + 1, out[Fields::OPLEN]);
	addEncoded((checkSequence ? eo.seq.size() : 0) + 1, out[Fields::SEQEND]);

	ZAMAN_END(Compress_EditOperation_AddEditOperation);
}

void EditOperationCompressor::addOperation (char op, int seqPos, int size, vector<Array<uint8_t>> &out) 
{
	ZAMAN_START(Compress_EditOperation_AddOperation);

	if (op == '=' || op == '*') 
		return;

	seqPos++; // Avoid zeros
	addEncoded(seqPos, out[Fields::SEQPOS]);
	if (!size) {
		out[Fields::OPCODES].add('0');
		out[Fields::OPCODES].add(op);
	} else if (op == 'X') {
		out[Fields::OPCODES].add(op); 
		addEncoded(size, out[Fields::XLEN]);
	} else if (op == 'H' || op == 'S') {
		out[Fields::OPCODES].add(op); 
		addEncoded(size, out[Fields::HSLEN]);
	} else {
		out[Fields::OPCODES].add(op); 
		addEncoded(size, out[Fields::LEN]);
	}

	ZAMAN_END(Compress_EditOperation_AddOperation);
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

