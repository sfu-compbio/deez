#include "EditOperation.h"
#include "Sequence.h"
using namespace std;

EditOperationDecompressor::EditOperationDecompressor (int blockSize, const SequenceDecompressor &seq):
	GenericDecompressor<EditOperation, GzipDecompressionStream>(blockSize),
	sequence(seq)
{
	streams.resize(EditOperationCompressor::Fields::ENUM_COUNT);
	for (int i = 0; i < streams.size(); i++)
		streams[i] = make_shared<GzipDecompressionStream>();
	streams[EditOperationCompressor::Fields::LOCATION] = make_shared<ArithmeticOrder0DecompressionStream<256>>();
}

EditOperationDecompressor::~EditOperationDecompressor (void) 
{
}

void EditOperationDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	ZAMAN_START(EditOperation);

	Array<uint8_t> stitches;
	decompressArray(streams[EditOperationCompressor::Fields::STITCH], in, stitches);
	Array<uint8_t> locations;
	size_t sz = decompressArray(streams[EditOperationCompressor::Fields::LOCATION], in, locations);

	REPEAT(3) {
		nucleotides[_] = ACTGStream();
		decompressArray(streams[EditOperationCompressor::Fields::ACGT + _], in, nucleotides[_].seqvec);
		decompressArray(streams[EditOperationCompressor::Fields::ACGT + _], in, nucleotides[_].Nvec);
		nucleotides[_].initDecode();
	}

	vector<Array<uint8_t>> oa(EditOperationCompressor::Fields::ACGT);
	vector<uint8_t*> fields(oa.size(), 0);
	for (int i = 2; i < oa.size(); i++)  {
		decompressArray(streams[i], in, oa[i]);
		fields[i] = oa[i].data();
	}

	records.resize(0);

	size_t stitchIdx = 0;
	uint32_t lastLoc = 0;
	for (size_t i = 0; i < sz; i++) {
		if (locations.data()[i] == 255)
			lastLoc = ((uint32_t*)stitches.data())[stitchIdx++];
		else
			lastLoc += locations.data()[i];
		records.add(getEditOperation(lastLoc, fields));
	}

	ZAMAN_END(EditOperation);
}

inline EditOperation EditOperationDecompressor::getEditOperation (size_t loc, vector<uint8_t*> &fields) 
{
	ZAMAN_START(GetEO);

	EditOperation eo;
	eo.start = eo.end = loc;
	eo.ops.resize(0);
	eo.seq = "";
	
	size_t prevLoc = 0, endPos = 0; 
	int count = getEncoded(fields[EditOperationCompressor::Fields::OPLEN]) - 1;
	for (int i = 0; i < count; i++) {
		char c = *fields[EditOperationCompressor::Fields::OPCODES]++;
		assert(c);
		int l = 0;
		if (c == '*') { // Unmapped case. Just add * and exit
			assert(count == 1);
			endPos = getEncoded(fields[EditOperationCompressor::Fields::SEQEND]) - 1;
			eo.ops.add(make_pair('*', endPos));
			nucleotides[2].get(eo.seq, endPos);
			ZAMAN_END(GetEO);
			return eo;
		} 
		
		endPos += getEncoded(fields[EditOperationCompressor::Fields::SEQPOS]) - 1;
		if (c == '0') {
			c = *fields[EditOperationCompressor::Fields::OPCODES]++, l = 0; i++;
		} else if (c == 'X') {
			l = getEncoded(fields[EditOperationCompressor::Fields::XLEN]);
		} else if (c == 'H' || c == 'S') {
			l = getEncoded(fields[EditOperationCompressor::Fields::HSLEN]);
		} else {
			l = getEncoded(fields[EditOperationCompressor::Fields::LEN]);
		}

		// Do  we have trailing = ?
		if ((c == 'N' || c == 'D' || c == 'H' || c == 'P') && endPos > prevLoc) {
			eo.ops.add(make_pair('=', endPos - prevLoc));
			eo.end += endPos - prevLoc;
		} else if (c != 'N' && c != 'D' && c != 'H' && c != 'P' && endPos - l > prevLoc) {
			eo.ops.add(make_pair('=', endPos - l - prevLoc));
			eo.end += endPos - l - prevLoc;
		}
		prevLoc = endPos;
		eo.ops.add(make_pair(c, l));
		if (c != 'S' && c != 'I' && c != 'H' && c != 'P') {
			if (c == 'X') {
				nucleotides[0].get(eo.seq, l);
			}
			eo.end += l;
		} else if (c == 'I' || c == 'S') {
			nucleotides[1].get(eo.seq, l);
		}
	}
	// End case. Check is prevLoc at end. If not, add = and exit
	endPos = getEncoded(fields[EditOperationCompressor::Fields::SEQEND]) - 1;
	if (!endPos)
		eo.seq += "*";
	else if (endPos > prevLoc) {
		eo.ops.add(make_pair('=', endPos - prevLoc));
		eo.end += endPos - prevLoc;
	}

	ZAMAN_END(GetEO);
}

EditOperation &EditOperationDecompressor::getRecord(size_t i) 
{
	ZAMAN_START(EditOperationGet);

	EditOperation &eo = records[i];

	size_t genPos = eo.start;
	char lastOP = 0;
	int  lastOPSize = 0;
	int mdOperLen = 0;
	eo.NM = 0;
	
	size_t eoSeqPos = 0;
	string seq;

	for (auto &op: eo.ops) {
		// restore original part
		switch (op.first) {
			case '=':
				if (lastOP == 'M')
					lastOPSize += op.second;
				else 
					lastOP = 'M', lastOPSize = op.second;
				
				for (int j = 0; j < op.second; j++) {
					seq += sequence[genPos + j];
					if (sequence[genPos + j] != sequence.getReference()[genPos + j]) {
						inttostr(mdOperLen, eo.MD), mdOperLen = 0;
						eo.MD += sequence.getReference()[genPos + j];
						eo.NM++;
					} else {
						mdOperLen++;
					}
				}
				genPos += op.second;
				break;

			case 'X':
				if (lastOP == 'M')
					lastOPSize += op.second;
				else 
					lastOP = 'M', lastOPSize = op.second;
				seq += eo.seq.substr(eoSeqPos, op.second);
				eoSeqPos += op.second;
				
				for (int j = 0; j < op.second; j++) {
					if (sequence.getReference()[genPos + j] != seq[seq.size() - op.second + j]) {
						inttostr(mdOperLen, eo.MD), mdOperLen = 0;
						eo.MD += sequence.getReference()[genPos + j];
						eo.NM++;
					} else {
						mdOperLen++;
					}
				}

				genPos += op.second;
				break;

			case 'I':
				eo.NM += op.second;
			case 'S':
			case '*':
				seq += eo.seq.substr(eoSeqPos, op.second);
				eoSeqPos += op.second;
			case 'H':
			case 'P':
				if (lastOP != 0) {
					inttostr(lastOPSize, eo.op); eo.op += lastOP;
				//	if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
				//		eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				if (op.first == '*') {
					eo.op = "*";
				} else {
					inttostr(op.second, eo.op); eo.op += op.first;
				}
				break;

			case 'D':
				inttostr(mdOperLen, eo.MD), mdOperLen = 0;
				eo.MD += "^";
				for (int j = 0; j < op.second; j++)
					eo.MD += sequence.getReference()[genPos + j]; 
				eo.NM += op.second;
			case 'N': 
				if (lastOP != 0) {
					inttostr(lastOPSize, eo.op); eo.op += lastOP;
				//	if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
				//		eo.end += lastOPSize;
					lastOP = 0, lastOPSize = 0;
				}
				inttostr(op.second, eo.op); eo.op += op.first;
				// eo.end += op.second;
				genPos += op.second;
				break;

			default:
				throw DZException("Invalid CIGAR opcode %c:%d", op.first, op.second);
		}
	}
	if (lastOP != 0) {
		inttostr(lastOPSize, eo.op); eo.op += lastOP;
	//	if (lastOP != 'S' && lastOP != 'I' && lastOP != 'H' && lastOP != 'P')
	//		eo.end += lastOPSize;
	}
	if (mdOperLen || !isdigit(eo.MD.back())) inttostr(mdOperLen, eo.MD);
	if (!isdigit(eo.MD[0])) eo.MD = "0" + eo.MD;

	if (seq == "" || eo.seq.back() == '*')
		seq = "*";
	eo.seq = seq;

	ZAMAN_END(EditOperationGet);
	return eo;
}

void EditOperationDecompressor::setIndexData (uint8_t *in, size_t in_size) 
{
	streams[EditOperationCompressor::Fields::LOCATION]->setCurrentState(in, in_size);
}

