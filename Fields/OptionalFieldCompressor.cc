#include "OptionalField.h"
using namespace std;

OptionalFieldCompressor::OptionalFieldCompressor (int blockSize):
	StringCompressor<GzipCompressionStream<6> >(blockSize),
	totalNM(0), totalMD(0), totalXD(0),
	failedNM(0), failedMD(0), failedXD(0) 
{
	indexStream = new GzipCompressionStream<6>();
}

OptionalFieldCompressor::~OptionalFieldCompressor (void) 
{
	DEBUG("NM stats: %'d failed out of %'d", failedNM, totalNM);
	DEBUG("MD stats: %'d failed out of %'d", failedMD, totalMD);
	DEBUG("XD stats: %'d failed out of %'d", failedXD, totalXD);

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

void OptionalFieldCompressor::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k,
	EditOperationCompressor *ec) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());
	ZAMAN_START(Compress_OptionalField);

	vector<Array<uint8_t>> oa;
	Array<uint8_t> buffer(k * 10, MB);
	Array<uint8_t> sizes(k, MB);

	for (size_t i = 0; i < k; i++) {
		int size = processFields(this->records[i], oa, buffer, (*ec)[i]);
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
	ZAMAN_END(Compress_OptionalField);
}

void OptionalFieldCompressor::parseMD(const string &rec, int &i, const string &eoMD, Array<uint8_t> &out)
{
	ZAMAN_START(Compress_OptionalField_ParseMD);
	totalMD++;
	string MD = "";
	for (; i < rec.size() && rec[i] != '\t'; i++)
		MD += rec[i];
	if (MD != eoMD) {
		//LOG("MD calculation failed: calculated %s, found %s", eoMD.c_str(), MD.c_str());
		failedMD++;
		out.add((uint8_t*)MD.c_str(), MD.size());
	}
	out.add(0);
	ZAMAN_END(Compress_OptionalField_ParseMD);
}

string OptionalFieldCompressor::getXDfromMD(const string &eoMD)
{
	ZAMAN_START(OptionalField_ParseXD);
	
	string XD;
	bool inDel= 0;
	for (int j = 0; j < eoMD.size(); j++) {
		if (eoMD[j] == '^') inDel = 1;
		if (isdigit(eoMD[j]) && inDel) inDel = 0, XD += '$';
		if (eoMD[j] != '0' || (j && isdigit(eoMD[j - 1])) || (j != eoMD.size() - 1 && isdigit(eoMD[j + 1])))
			XD += eoMD[j];
	}

	ZAMAN_END(OptionalField_ParseXD);
	return XD;
}

void OptionalFieldCompressor::parseXD(const string &rec, int &i, const string &eoMD, Array<uint8_t> &out)
{
	ZAMAN_START(Compress_OptionalField_ParseXD);
	totalXD++;

	string eoXD = getXDfromMD(eoMD), XD;
	for (; i < rec.size() && rec[i] != '\t'; i++)
		XD += rec[i];
	if (XD != eoXD) {
		DEBUG("XD calculation failed: calculated %s, found %s", eoXD.c_str(), XD.c_str());
		failedXD++;
		out.add((uint8_t*)XD.c_str(), XD.size());
	}
	out.add(0);
	ZAMAN_END(Compress_OptionalField_ParseXD);
}

int OptionalFieldCompressor::processFields (const string &rec, vector<Array<uint8_t>> &out, 
	Array<uint8_t> &tags, const EditOperation &eo) 
{
	ZAMAN_START(Compress_OptionalField_ParseFields);
	Array<uint8_t> packedInt(8); 
	int size = 0;
	int prevKey = 0, tagStartIndex = tags.size();
	for (int i = 0; i < rec.size(); i++) {
		if (rec.size() - i < 5 || rec[i + 2] != ':' || rec[i + 4] != ':')
			throw DZException("Invalid SAM tag %s", rec.substr(i).c_str());

		string key = rec.substr(i, 2) + rec[i + 3];
		uint64_t num;
		switch (key[2]) { // check exact type!
		case 'c': case 'C': 
			num = (uint8_t)atoi(rec.c_str() + i + 5);
		case 's': case 'S': 
			num = (uint16_t)atoi(rec.c_str() + i + 5);
		case 'i': case 'I':
			num = (uint32_t)atoi(rec.c_str() + i + 5);
			int ret = packInteger(num, packedInt);
			key += char('0' + ret);
			break;
		}

		if (eo.NM != -1 && key.substr(0, 2) == "NM" && eo.NM == num) {
			key = "NMi"; // this is unstored NM
		}
		
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

		i += 5;
		if (key == "MDZ") {
			parseMD(rec, i, eo.MD, out[keyIndex]);
		} else if (key == "XDZ") {
			parseXD(rec, i, eo.MD, out[keyIndex]);
		} else if (key == "NMi") {
		} else switch (key[2]) {
			case 'c': case 'C':
			case 's': case 'S':
			case 'i': case 'I':
				out[keyIndex].add(packedInt.data(), packedInt.size());
				break;
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
				for (; i < rec.size() && rec[i] != '\t'; i++)
					out[keyIndex].add(rec[i]);
				out[keyIndex].add(0);
				break;
		}
		for (; rec[i] != '\t' && i < rec.size(); i++);
		packedInt.resize(0);
		size++;
	}
	ZAMAN_END(Compress_OptionalField_ParseFields);
	return size;
}
