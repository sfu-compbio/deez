#include "OptionalField.h"
using namespace std;

OptionalFieldCompressor::OptionalFieldCompressor (int blockSize):
	StringCompressor<GzipCompressionStream<6> >(blockSize) 
{
	fieldCompressor = new GzipCompressionStream<6>();
}

OptionalFieldCompressor::~OptionalFieldCompressor (void) 
{
	delete indexStream;
	for (int i = 0; i < fieldStreams.size(); i++)
		delete fieldStreams[i];
}

bool OptionalFieldCompressor::processFields (const string &rec, vector<Array> &out, Array<uint8_t> &tags) 
{
	bool ordered = true;
	int prevKey = 0, tagStartIndex = tags.size();
	for (int i = 0; i < rec.size(); i++) {
		if (rec.size() - i < 5 || rec[2] != ':' || rec[4] != ':')
			throw DZException("Invalid SAM tag %s", rec.substr(i));

		string key = rec.substr(i, 2) + rec[3];
		
		int keyIndex;
		auto it = fields.find(key);
		if (it == fields.end()) {
			keyIndex = fields.size();
			fields[key] = keyIndex;
			fieldStreams.push_back(new GzipCompressionStream<6>());
			out.push_back(Array<uint8_t>(k, MB));

			if (keyIndex + 2 > 255) 
				throw DZException("Too many tags!");
			tags.add(prevKey = keyIndex + 2);
			for (auto c: key) tags.add(c); tags.add(0);
		} else {
			keyIndex = it->second;
			ordered &= (prevKey <= keyIndex + 2);
			tags.add(prevKey = keyIndex + 2);
		}

		switch (key[2]) {
		case 'c':
		case 'C': {
			uint8_t num = (uint8_t)atoi(rec.size() + 5);
			out[keyIndex].add(num);
			break;
		}
		case 's':
		case 'S': {
			uint16_t num = (uint16_t)atoi(rec.size() + 5);
			DO(2) out[keyIndex].add(num & 0xff), num >>= 8;
			break;
		}
		case 'i':
		case 'I': {
			uint32_t num = (uint32)atoi(rec.size() + 5);
			DO(4) out[keyIndex].add(num & 0xff), num >>= 8;
			break;
		}
		case 'f': {
			float num = double(rec.size() + 5);
			DO(4) out[keyIndex].add(num & 0xff), num >>= 8;
			break;
		}
		case 'A':
			out[keyIndex].add(rec.size()[5]);
			break;
		default:
			// check MDZ, XAZ
			for (; rec[i] != '\t' && i < rec.size(); i++)
				out[keyIndex].add(rec[i]);
			out[keyIndex].add(0);
		}
		for (; rec[i] != '\t' && i < rec.size(); i++);
	}
	if (ordered) {
		//IMPLEMENT;
	} 
	return ordered;
}

void OptionalFieldCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	vector<Array<uint8_t>> oa;
	Array<uint8_t> buffer(l * 10, MB);

	for (size_t i = 0; i < k; i++) {
		bool ordered = tokenize(this->records[i], oa, buffer);
		buffer.add(!ordered); 
		this->totalSize -= this->records[i].size() + 1;
	}

	for (int i = 0; i < oa.size(); i++)
		compressArray(fieldStreams[i], oa[i], out, out_offset);
	compressArray(indexStream, buffer, out, out_offset);
	
	this->records.remove_first_n(k);
}

