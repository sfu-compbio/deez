#include "EditOperation.h"
#include "Sequence.h"
#include "../Streams/rANSOrder2Stream.h"

using namespace std;

EditOperation::EditOperation(size_t s, const std::string &se, const std::string &op) :
	start(s), end(s), seq(se), NM(-1), ops(2, op.size() / 2), op(op)
{
	if (op == "*") {
		ops.add(make_pair('*', 0));
		return;
	}

	for (size_t pos = 0, size = 0; pos < op.length(); pos++) {
		if (isdigit(op[pos])) {
			size = size * 10 + (op[pos] - '0');
			continue;
		}
		switch (op[pos]) {
		case 'M':
		case '=':
		case 'X':
			end += size;
			break;
		case 'D':
		case 'N':
			end += size;
			break;
		case 'I':
		case 'S':
		case 'H':
		case 'P':
			break;
		default:
			throw DZException("Bad CIGAR detected: %c in %s", op[pos], op.c_str());
		}
		ops.add(make_pair(op[pos], size));
		size = 0;
	}
	//LOG("%d %d", start, end);
	assert(ops.size());
}

void EditOperation::calculateTags(Reference &reference) 
{
	ZAMAN_START(CalculateMDNM);
	NM = 0;
	size_t mdOperLen = 0, seqPos = 0, genPos = start;
	for (auto &op: ops) {
		switch (op.first) {
		case 'M':
		case '=':
		case 'X':
			for (size_t i = 0; i < op.second; i++, seqPos++, genPos++) {
				if (reference[genPos] != seq[seqPos]) {
					inttostr(mdOperLen, MD), mdOperLen = 0;
					MD += reference[genPos];
					NM++;
				} else {
					mdOperLen++;
				}
			}
			break;
		case 'D':
			inttostr(mdOperLen, MD), mdOperLen = 0;
			MD += "^";
			for (size_t i = 0; i < op.second; i++) 
				MD += reference[genPos + i]; 
			NM += op.second;
		case 'N':
			genPos += op.second;
			break;
		case 'I':
			NM += op.second;
		case 'S':
			seqPos += op.second;
			break;
		}
	}
	if (mdOperLen || !isdigit(MD.back())) 
		inttostr(mdOperLen, MD);
	if (!isdigit(MD[0])) 
		MD = "0" + MD;

	ZAMAN_END(CalculateMDNM);
	assert(ops.size());
}

template<>
size_t sizeInMemory(EditOperation t) {
	return sizeof(t) + 
		sizeInMemory(t.seq) +
		sizeInMemory(t.MD) +
		sizeInMemory(t.op) +
		sizeInMemory(t.ops) -
		3 * sizeof(std::string) - sizeof(t.ops); 
}

EditOperationCompressor::EditOperationCompressor (const SequenceCompressor &seq):
	GenericCompressor<EditOperation, GzipCompressionStream<6>>(),
	sequence(seq)
{
	streams.resize(Fields::ENUM_COUNT);
	for (int i = 0; i < streams.size(); i++)
		streams[i] = make_shared<GzipCompressionStream<6>>();
	streams[Fields::LOCATION] = make_shared<ArithmeticOrder0CompressionStream<256>>();
}

void EditOperationCompressor::printDetails(void) 
{
	vector<const char*> s { "Positions", "Stitches", "Opcode", "SeqPos", "SeqLen", "XLen", "HSLen", "AnyLen", "OPLen", 
		"ACGTMiss", "ACGTIns", "ACGTUnmap" };
	for (int i = 0; i < streams.size(); i++) 
		LOG("  %-10s: %'20lu ", s[i], streams[i]->getCount());
}

void EditOperationCompressor::outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k, const Array<EditOperation> &editOps) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	ZAMAN_START(EditOperationOutput);

	vector<Array<uint8_t>> oa(streams.size());
	for (int i = 2; i < Fields::ACGT; i++) 
		oa[i] = Array<uint8_t>(k, MB);
	Array<uint8_t> locations(k);
	Array<uint32_t> stitches(k);	

	ACTGStream nucleotides[3];
	REPEAT(3) { // Mismatch; InsClip; Unmapped
		nucleotides[_] = ACTGStream(k * records[0].getSequenceSize(), MB); 
		nucleotides[_].initEncode();
	}

	ZAMAN_START(AddEditOperation);

	uint32_t lastLoc = 0;
	for (size_t i = 0; i < k; i++) {
		int loc = records[i].getLocation();
		if (loc == (size_t)-1) 
			loc = 0;
		if (loc - lastLoc >= 254)
			locations.add(255), stitches.add(loc);
		else
			locations.add(loc - lastLoc);
		lastLoc = loc;

		addEditOperation(records[i], editOps[i], nucleotides, oa);
	}

	ZAMAN_END(AddEditOperation);
	
	compressArray(streams[Fields::STITCH], stitches, out, out_offset);
	compressArray(streams[Fields::LOCATION], locations, out, out_offset);

	ZAMAN_START(ACGT);
	REPEAT(3) { 
		nucleotides[_].flush();
		compressArray(streams[Fields::ACGT + _], nucleotides[_].seqvec, out, out_offset);
		compressArray(streams[Fields::ACGT + _], nucleotides[_].Nvec, out, out_offset);
	}
	ZAMAN_END(ACGT);

	for (int i = 2; i < Fields::ACGT; i++)
		compressArray(streams[i], oa[i], out, out_offset);

	ZAMAN_END(EditOperationOutput);
}

void EditOperationCompressor::addEditOperation(const Record &record, const EditOperation &eo, ACTGStream *nucleotides, vector<Array<uint8_t>> &out) 
{
	const char *seq = record.getSequence();
	size_t seqSize = record.getSequenceSize();
	
	if (eo.ops[0].first == '*') {
		if (seq[0] != '*') {
			nucleotides[2].add(seq, seqSize);
			addEncoded(seqSize + 1, out[Fields::SEQEND]);
		} else {
			addEncoded(1, out[Fields::SEQEND]);
		}
		out[Fields::OPCODES].add('*');
		addEncoded(1 + 1, out[Fields::OPLEN]);
		return;
	}

	size_t size   = 0;
	size_t genPos = record.getLocation();
	size_t seqPos = 0, prevSeqPos = 0;

	char lastOP = 0;
	int  lastOPSize = 0;

	bool checkSequence = seq[0] != '*';
	int opcodeOffset = out[Fields::OPCODES].size();
	for (auto &op: eo.ops) {
		lastOP = lastOPSize = 0;
		switch (op.first) {
		case 'M': // any; will become =X in DeeZ's internal structure
		case '=': // match
		case 'X': // mismatch
			if (!op.second) {
				lastOP = 'X', lastOPSize = 0;
			}
			for (size_t i = 0; i < op.second; i++) {
				if (checkSequence && seq[seqPos] == sequence[genPos]) {
					if (lastOP == '=')
						lastOPSize++;
					else {
						if (lastOP && lastOP != '=') {
							addOperation(lastOP, seqPos - prevSeqPos, lastOPSize, out), prevSeqPos = seqPos;
						}
						lastOP = '=', lastOPSize = 1;
					}
				} else if (checkSequence && lastOP == 'X') {
					nucleotides[0].add(seq + seqPos, 1), lastOPSize++;
				} else {
					if (lastOP && lastOP != '=') {
						addOperation(lastOP, seqPos - prevSeqPos, lastOPSize, out), prevSeqPos = seqPos;
					}
					if (checkSequence) nucleotides[0].add(seq + seqPos, 1);
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
			if (checkSequence) nucleotides[1].add(seq + seqPos, op.second);	
			seqPos += op.second;
			addOperation(op.first, seqPos - prevSeqPos, op.second, out), prevSeqPos = seqPos;
			break;
		case 'D':
		case 'N':
			addOperation(op.first, seqPos - prevSeqPos, op.second, out), prevSeqPos = seqPos;
			genPos += op.second;
			break;
		case 'H':
		case 'P':
			addOperation(op.first, seqPos - prevSeqPos, op.second, out), prevSeqPos = seqPos;
			break;
		default:
			throw DZException("Bad CIGAR detected: %s", op.first);
		}
	}
	addEncoded(out[Fields::OPCODES].size() - opcodeOffset + 1, out[Fields::OPLEN]);
	addEncoded((checkSequence ? seqSize : 0) + 1, out[Fields::SEQEND]);
}

void EditOperationCompressor::addOperation (char op, int seqPos, int size, vector<Array<uint8_t>> &out) 
{
	//ZAMAN_START(AddOperation);

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

	//ZAMAN_END(AddOperation);
}

void EditOperationCompressor::getIndexData (Array<uint8_t> &out) 
{
	out.resize(0);
	streams[Fields::LOCATION]->getCurrentState(out);
}
