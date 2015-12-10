#include "OptionalField.h"
using namespace std;

OptionalFieldCompressor::OptionalFieldCompressor (int blockSize):
	StringCompressor<GzipCompressionStream<6> >(blockSize) 
{
	indexStream = new GzipCompressionStream<6>();
}

OptionalFieldCompressor::~OptionalFieldCompressor (void) 
{
	delete indexStream;
	for (auto &m: fieldStreams)
		delete m.second;
}

size_t OptionalFieldCompressor::compressedSize(void) 
{ 
	int res = 0;
	for (auto &m: fieldStreams)
		res += m.second->getCount();
	return indexStream->getCount() + res;
}

void OptionalFieldCompressor::printDetails(void) 
{
	LOG("  Index     : %'20lu", indexStream->getCount());
	for (auto &m: fieldStreams) 
		LOG("  %-10s: %'20lu", m.first.c_str(), m.second->getCount());
}

/*
string MDZ(const EditOperation &eo, const string &ref, const string &seq) 
{
	int correct = 0;
	for (size_t pos = 0, size = 0; pos < eo.op.size(); pos++) {
		if (isdigit(eo.op[pos])) {
			size = size * 10 + (eo.op[pos] - '0');
			continue;
		}

		switch (eo.op[pos]) {
			case 'M': 
			case '=': 
			case 'X': 
				for (size_t i = 0; i < size; i++, seqPos++) {
					if (eo.seq[seqPos] == fixed[genPos]) {
						correct++;
					} else {
						if (correct)
							result += inttostr(correct);
						result += eo.seq[seqPos];
						correct = 0;
					}
				}
				break;
			case 'I':
			case 'S':
				result += "^" + nucleotides.add(eo.seq.c_str() + seqPos, size);	
				if (XD) result += "$";
				seqPos += size;
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

}
*/
int OptionalFieldCompressor::processFields (const string &rec, vector<Array<uint8_t>> &out, Array<uint8_t> &tags) 
{
	int size = 0;
	int prevKey = 0, tagStartIndex = tags.size();
	for (int i = 0; i < rec.size(); i++) {
		if (rec.size() - i < 5 || rec[i + 2] != ':' || rec[i + 4] != ':')
			throw DZException("Invalid SAM tag %s", rec.substr(i).c_str());

		string key = rec.substr(i, 2) + rec[i + 3];
		
		int keyIndex;
		auto it = fields.find(key);
		if (it == fields.end()) {
			keyIndex = fields.size();
			fields[key] = keyIndex;
			out.push_back(Array<uint8_t>());
			out.back().set_extend(MB);

			addEncoded(prevKey = keyIndex + 1, tags);
			for (auto c: key) tags.add(c); tags.add(0);
		} else {
			keyIndex = it->second;
			addEncoded(prevKey = keyIndex + 1, tags);
		}
		size++;

		i += 5;
		switch (key[2]) {
		case 'c':
		case 'C': {
			uint8_t num = (uint8_t)atoi(rec.c_str() + i);
			out[keyIndex].add(num);
			break;
		}
		case 's':
		case 'S': {
			uint16_t num = (uint16_t)atoi(rec.c_str() + i);
			REPEAT(2) out[keyIndex].add(num & 0xff), num >>= 8;
			break;
		}
		case 'i':
		case 'I': {
			uint32_t num = (uint32_t)atoi(rec.c_str() + i);
			REPEAT(4) out[keyIndex].add(num & 0xff), num >>= 8;
			break;
		}
		case 'f': {
			float num = atof(rec.c_str() + i);
			uint8_t *np = (uint8_t*)(&num);
			REPEAT(4) out[keyIndex].add(*np), np++;
			break;
		}
		case 'A':
			out[keyIndex].add(rec[i]);
			break;
		default:
			// check MDZ, XAZ
			for (; i < rec.size() && rec[i] != '\t'; i++)
				out[keyIndex].add(rec[i]);
			out[keyIndex].add(0);
			break;
		}
		for (; rec[i] != '\t' && i < rec.size(); i++);
	}
	return size;
}

void OptionalFieldCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	vector<Array<uint8_t>> oa;
	Array<uint8_t> buffer(k * 10, MB);
	Array<uint8_t> sizes(k, MB);

	for (size_t i = 0; i < k; i++) {
		int size = processFields(this->records[i], oa, buffer);
		addEncoded(size + 1, sizes);
		this->totalSize -= this->records[i].size() + 1;
	}

	compressArray(indexStream, buffer, out, out_offset);
	compressArray(indexStream, sizes, out, out_offset);

	unordered_map<int, string> mm;
	for (auto &f: fields) mm[f.second] = f.first;
	for (int i = 0; i < oa.size(); i++) {
		string key = mm[i];
		if (fieldStreams.find(key) == fieldStreams.end())
			fieldStreams[key] = new GzipCompressionStream<6>();
		compressArray(fieldStreams[key], oa[i], out, out_offset);
	}
	
	this->records.remove_first_n(k);
	fields.clear();
}

OptionalFieldDecompressor::OptionalFieldDecompressor (int blockSize):
	StringDecompressor<GzipDecompressionStream> (blockSize)
{
	indexStream = new GzipDecompressionStream();
}

OptionalFieldDecompressor::~OptionalFieldDecompressor (void) 
{
	delete indexStream;
	for (auto &m: fieldStreams)
		delete m.second;
}

void OptionalFieldDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	Array<uint8_t> index, sizes;
	size_t s = decompressArray(indexStream, in, index);
	decompressArray(indexStream, in, sizes);
	uint8_t *tags = index.data();
	uint8_t *tagsEnd = tags + s;
	uint8_t *szs = sizes.data();

	vector<Array<uint8_t>> oa;
	vector<ssize_t> out;
	std::unordered_map<int, std::string> fields;

	records.resize(0);
	while (szs != sizes.data() + sizes.size()) {
		string result;
		int size = getEncoded(szs) - 1;
		while (size--) {
			string key;
			int keyIndex = getEncoded(tags) - 1;
			auto it = fields.find(keyIndex);
			if (it == fields.end()) {
				while (*tags) key += *tags++; tags++;
				fields[keyIndex] = key; 
				assert(oa.size() == keyIndex);
				assert(out.size() == keyIndex);
				if (fieldStreams.find(key) == fieldStreams.end())
					fieldStreams[key] = new GzipDecompressionStream();
				oa.push_back(Array<uint8_t>());
				decompressArray(fieldStreams[key], in, oa[keyIndex]);
				out.push_back(0);
			} else {
				key = it->second;
			}

			if (result != "") result += "\t";
			result += S("%c%c:%c:", key[0], key[1], key[2]);
			switch (key[2]) {
				case 'c':
					result += S("%d", int8_t(oa[keyIndex].data()[out[keyIndex]++]));
					break;
				case 'C':
					result += S("%d", uint8_t(oa[keyIndex].data()[out[keyIndex]++]));
					break;
				case 's': {
					int16_t num = 0;
					REPEAT(2) num |= (oa[keyIndex].data()[out[keyIndex]++]) << (8 * _);
					result += S("%d", num);
					break;
				}
				case 'S': {
					uint16_t num = 0;
					REPEAT(2) num |= (oa[keyIndex].data()[out[keyIndex]++]) << (8 * _);
					result += S("%d", num);
					break;
				}
				case 'i': {
					int32_t num = 0;
					REPEAT(4) num |= (oa[keyIndex].data()[out[keyIndex]++]) << (8 * _);
					result += S("%d", num);
					break;
				}
				case 'I': {
					uint32_t num = 0;
					REPEAT(4) num |= (oa[keyIndex].data()[out[keyIndex]++]) << (8 * _);
					result += S("%u", num);
					break;
				}
				case 'f': {
					uint32_t num = 0;
					REPEAT(4) num |= (oa[keyIndex].data()[out[keyIndex]++]) << (8 * _);
					result += S("%g", *((float*)(&num)));
					break;
				}
				case 'A':
					result += (char)(oa[keyIndex].data()[out[keyIndex]++]);
					break;
				default: {
					while (oa[keyIndex].data()[out[keyIndex]])
						result += oa[keyIndex].data()[out[keyIndex]++];
					out[keyIndex]++;
					break;
				}
			}
		}
		records.add(result);
	}

	recordCount = 0;
}

