#include "Fields.h"
#include <cstring>
using namespace std;

extern char optQuality;
extern char optLossy;
extern bool optNoQual;

namespace Legacy {
namespace v11 {

/****************************************************************************************/

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

/****************************************************************************************/

PairedEndDecompressor::PairedEndDecompressor (int blockSize):
	GenericDecompressor<PairedEndInfo, GzipDecompressionStream>(blockSize)
{
}

PairedEndDecompressor::~PairedEndDecompressor (void) {
}

const PairedEndInfo &PairedEndDecompressor::getRecord (const string &mc, size_t mpos) {
	assert(hasRecord());
	PairedEndInfo &p = records.data()[recordCount++];
	if (mc == p.chr) {
		if (p.tlen <= 0)
			p.pos += mpos;
		else
			p.pos = mpos - p.pos;
	}
	p.pos--;
	return p;
}

void PairedEndDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) 
		return;
	assert(in_size >= sizeof(size_t));
	
	Array<uint8_t> au;
	size_t s = decompressArray(stream, in, au);

	PairedEndInfo pe;
	size_t vi = 0;
	records.resize(0);
	std::map<char, std::string> chromosomes;
	char chr;
	for (size_t i = 0; i < s; ) {
		pe.pos = *(uint16_t*)(au.data() + i), i += sizeof(uint16_t);
		if (!pe.pos) 
			pe.pos = *(uint32_t*)(au.data() + i), i += sizeof(uint32_t);
		pe.tlen = *(int32_t*)(au.data() + i), i += sizeof(int32_t);
		chr = *(au.data() + i), i++;
		if (chr == -1) {
			string sx = "";
			while (au.data()[i])
				sx += au.data()[i++];
			i++;
			chr = chromosomes.size();
			pe.chr = chromosomes[chr] = sx;
		}
		else pe.chr = chromosomes[chr];	
		records.add(pe);
	}
	
	recordCount = 0;
}

/****************************************************************************************/

QualityScoreDecompressor::QualityScoreDecompressor (int blockSize):
	StringDecompressor<QualityDecompressionStream>(blockSize) 
{
	sought = 0;
	switch (optQuality) {
		case 0:
			break;
		case 1:
			delete this->stream;
			this->stream = new SAMCompStream<QualRange>();
			sought = 1;
			break;
		case 2:
			break;
	}
	const char* qualities[] = { "default", "samcomp", "test" };
	DEBUG("Using quality mode %s", qualities[optQuality]);

	if (optNoQual)
		sought = 2;
}

QualityScoreDecompressor::~QualityScoreDecompressor (void) {
}

void QualityScoreDecompressor::setIndexData (uint8_t *in, size_t in_size) {
	stream->setCurrentState(in, in_size);
	if (sought) 
		sought = 2;
}

string QualityScoreDecompressor::getRecord (size_t seq_len, int flag) {
	if (sought == 2) {
		string s = "";
		for (int i = 0; i < seq_len; i++)
			s += (char)64;
		return s;
	}

	assert(hasRecord());
	
	string s = string(StringDecompressor<QualityDecompressionStream>::getRecord());
	if (s == "")
		return "*";

	for (size_t i = 0; i < s.size(); i++)
		s[i] += offset - 1;
    char c = s[s.size() - 1];
    while (s.size() < seq_len)
		s += c;
    if (flag & 0x10) for (size_t i = 0; i < seq_len / 2; i++)
		swap(s[i], s[seq_len - i - 1]);
    return s;
}

void QualityScoreDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) 
		return;
	if (sought == 2)
		return;

	offset = *in++;
	StringDecompressor<QualityDecompressionStream>::importRecords(in, in_size - 1);
}

/****************************************************************************************/

const int MAX_TOKEN = 20;

ReadNameDecompressor::ReadNameDecompressor (int blockSize):
	StringDecompressor<GzipDecompressionStream> (blockSize)
{
	indexStream = new GzipDecompressionStream();
}

ReadNameDecompressor::~ReadNameDecompressor (void) {
	delete indexStream;
}

void ReadNameDecompressor::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) return;

	Array<uint8_t> index;
	size_t s1 = decompressArray(indexStream, in, index);
	Array<uint8_t> content;
	decompressArray(stream, in, content);

	string tokens[MAX_TOKEN];
	size_t ic = 0, cc = 0;

	records.resize(0);
	string prevTokens[MAX_TOKEN];
	char prevCharTokens[MAX_TOKEN] = { 0 };
	char charTokens[MAX_TOKEN] = { 0 };

	while (ic < s1) {
		int prevTokenID = -1;
		int T, t, prevT = 0;
		while (1) {
			t = (T = index.data()[ic++]) % MAX_TOKEN;
			
			if (t == prevTokenID) {
				charTokens[t] = decodeUInt(0, content, cc);
				prevCharTokens[t] = charTokens[t];
		//		LOGN("+[%c]", charTokens[t]);
				continue;
			}
			prevTokenID = t;

			while (prevT < t) {
				tokens[prevT] = prevTokens[prevT];
				charTokens[prevT] = prevCharTokens[prevT]; 
				prevT++;
			}
			T /= MAX_TOKEN;
			if (T >= 6)  
				break;
			switch (T) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 4: {
					uint64_t e = decodeUInt(T, content, cc);
					tokens[t] = "";
					if (!e) tokens[t] = "0";
					while (e) tokens[t] = char('0' + e % 10) + tokens[t], e /= 10;
					break;
				}
				case 5: 
					tokens[t] = "";
					while (content.data()[cc])  
						tokens[t] += content.data()[cc++];
					cc++;
					break;
				default:
					throw DZException("Read name data corrupted");
			}
			prevTokens[t] = tokens[t];
			prevT = t;
		}
		if (t > 1 && charTokens[0] == 0 && tokens[0].size() == 3 // == '.' //&& tokens[0].size() && !isdigit(tokens[0][tokens[0].size() - 1]) 
				&& isalpha(tokens[0][2]) && tokens[1][0] >= '1' && tokens[1][0] <= '9') 
		{
			tokens[0] += tokens[1];
			for (int i = 2; i < t; i++)
				tokens[0] += charTokens[i - 1] + tokens[i];
		}
		else for (int i = 1; i < t; i++)
			tokens[0] += charTokens[i - 1] + tokens[i];
		records.add(tokens[0]);
	}

	recordCount = 0;
}

void ReadNameDecompressor::setIndexData (uint8_t *in, size_t in_size) {
	stream->setCurrentState(in, in_size);
}

/****************************************************************************************/

SequenceDecompressor::SequenceDecompressor (const string &refFile, int bs):
	reference(refFile), 
	chromosome(""),
	fixed(0)
{
	fixesStream = new GzipDecompressionStream();
	fixesReplaceStream = new GzipDecompressionStream();
}

SequenceDecompressor::~SequenceDecompressor (void)
{
	delete[] fixed;
	delete fixesStream;
	delete fixesReplaceStream;
}


bool SequenceDecompressor::hasRecord (void) 
{
	return true;
}

void SequenceDecompressor::importRecords (uint8_t *in, size_t in_size)  
{
	if (chromosome == "*" || in_size == 0)
		return;

	size_t newFixedStart = *(size_t*)in; in += sizeof(size_t);
	size_t newFixedEnd   = *(size_t*)in; in += sizeof(size_t);
	
	Array<uint8_t> fixes_loc;
	decompressArray(fixesStream, in, fixes_loc);
	Array<uint8_t> fixes_replace;
	decompressArray(fixesReplaceStream, in, fixes_replace);

	////////////
	// NEED FROM READSSTART
	assert(fixedStart <= newFixedStart);	
	char *newFixed = new char[newFixedEnd - newFixedStart];
	// copy old fixes!
	if (fixed && newFixedStart < fixedEnd) {
		memcpy(newFixed, fixed + (newFixedStart - fixedStart), 
			fixedEnd - newFixedStart);
		reference.load(newFixed + (fixedEnd - newFixedStart), 
			fixedEnd, newFixedEnd);
	}
	else reference.load(newFixed, newFixedStart, newFixedEnd);

	fixedStart = newFixedStart;
	fixedEnd = newFixedEnd;
	delete[] fixed;
	fixed = newFixed;
	
	size_t prevFix = 0;
	uint8_t *len = fixes_loc.data();
	for (size_t i = 0; i < fixes_replace.size(); i++) {
		prevFix += getEncoded(len) - 1;
		assert(prevFix < fixedEnd);
		assert(prevFix >= fixedStart);
		fixed[prevFix - fixedStart] = fixes_replace.data()[i];
	}
}

void SequenceDecompressor::setFixed (EditOperationDecompressor &editOperation) 
{
	editOperation.setFixed(fixed, fixedStart);
}

void SequenceDecompressor::scanChromosome (const string &s)
{
	delete[] fixed;
	fixed = 0;
	fixedStart = fixedEnd = 0;
	chromosome = reference.scanChromosome(s);
}

};
};
