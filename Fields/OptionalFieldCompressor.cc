#include "OptionalField.h"
#include "QualityScore.h"
#include "../Streams/rANSOrder2Stream.h"
using namespace std;

OptionalFieldCompressor::OptionalFieldCompressor(void):
	StringCompressor<GzipCompressionStream<6>>(),
	fields(AlphabetRange * AlphabetRange * AlphabetRange, -1),
	fieldCount(0),
	totalNM(0), totalMD(0), totalXD(0),
	failedNM(0), failedMD(0), failedXD(0) 
{
}

OptionalFieldCompressor::~OptionalFieldCompressor (void) 
{
	if (failedNM) LOG("NM stats: %'d failed out of %'d", failedNM, totalNM);
	if (failedMD) LOG("MD stats: %'d failed out of %'d", failedMD, totalMD);
	if (failedXD) LOG("XD stats: %'d failed out of %'d", failedXD, totalXD);
}

size_t OptionalFieldCompressor::compressedSize(void) 
{ 
	size_t res = 0;
	for (int i = 0; i < streams.size(); i++) 
		res += streams[i]->getCount();
	for (auto &m: fieldStreams) 
		res += m.second->getCount();
	return res;
}

void OptionalFieldCompressor::printDetails(void) 
{
	LOG("  Index     : %'20lu", streams[0]->getCount());
	for (auto &m: fieldStreams) 
		LOG("  %-10s: %'20lu", m.first.c_str(), m.second->getCount());
}

void OptionalFieldCompressor::outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k, const Array<EditOperation> &editOps) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());
	ZAMAN_START(OptionalFieldOutput);

	vector<Array<uint8_t>> oa;
	Array<uint8_t> buffer(k * 10, MB);
	Array<uint8_t> sizes(k, MB);

	for (size_t i = 0; i < k; i++) {
		int size = processFields(records[i].getOptional(), records[i].getOptional() + records[i].getOptionalSize(), oa, buffer, editOps[i]);
		addEncoded(size + 1, sizes);
		totalSize -= records[i].getOptionalSize() + 1;
	}

	ZAMAN_START(Index);
	compressArray(streams[0], buffer, out, out_offset);
	compressArray(streams[0], sizes, out, out_offset);
	ZAMAN_END(Index);

	ZAMAN_START(Keys);
	unordered_map<int, string> mm;
	for (int f = 0; f < fields.size(); f++) if (fields[f] != -1) {
		mm[fields[f]] = 
			string(1, AlphabetStart + (f / AlphabetRange) / AlphabetRange) +
			string(1, AlphabetStart + (f / AlphabetRange) % AlphabetRange) +
			string(1, AlphabetStart + f % AlphabetRange);
	}
	for (int i = 0; i < oa.size(); i++) {
		ZAMAN_START(C);
		string key = mm[i];
		__zaman_prefix__ = __zaman_prefix__.substr(0, __zaman_prefix__.size() - 2) + key;
		if (fieldStreams.find(key) == fieldStreams.end())
			//if (key == "OQZ") { // gzip is particularly bad with any kind of quality scores; even AC is faster than it
			//	fieldStreams[key] = make_shared<rANSOrder2CompressionStream<128>>();
			//} else {
				fieldStreams[key] = make_shared<GzipCompressionStream<6>>();
			//}
		compressArray(fieldStreams[key], oa[i], out, out_offset);
		ZAMAN_END(C);
		__zaman_prefix__ = __zaman_prefix__.substr(0, __zaman_prefix__.size() - key.size() + 2);
	}
	ZAMAN_END(Keys);
	
	fill(fields.begin(), fields.end(), -1);
	fieldCount = 0;
	PG.clear();
	RG.clear();
	ZAMAN_END(OptionalFieldOutput);
}

string OptionalFieldCompressor::getXDfromMD(const string &eoMD)
{
	string XD;
	bool inDel= 0;
	for (int j = 0; j < eoMD.size(); j++) {
		if (eoMD[j] == '^') inDel = 1;
		if (isdigit(eoMD[j]) && inDel) inDel = 0, XD += '$';
		if (eoMD[j] != '0' || (j && isdigit(eoMD[j - 1])) || (j != eoMD.size() - 1 && isdigit(eoMD[j + 1])))
			XD += eoMD[j];
	}
	return XD;
}

bool OptionalFieldCompressor::parseXD(const char *rec, const string &eoMD)
{
	totalXD++;
	string eoXD = getXDfromMD(eoMD);
	for (int i = 0; i < eoXD.size(); i++)
		if (eoXD[i] != rec[i]) 
			return false;
	if (rec[eoXD.size()])
		return false;
	return true;
}

int OptionalFieldCompressor::processFields (const char *rec, const char *recEnd, vector<Array<uint8_t>> &out, 
	Array<uint8_t> &tags, const EditOperation &eo) 
{
	ZAMAN_START(ParseFields);
	Array<uint8_t> packedInt(8); 
	int size = 0;
	int prevKey = 0;
	for (; rec < recEnd; rec++) {
		if (recEnd - rec < 5 || rec[2] != ':' || rec[4] != ':') {
			LOG("ooops... %s %d %d", rec, rec[0], recEnd - rec);
			throw DZException("Invalid SAM tag %s", rec);
		}

		char type = rec[3];
		int key = Field(rec[0], rec[1], rec[3]);
		rec += 5;

		uint64_t num;
		if (type == 'i') 
			num = (uint32_t)atoi(rec);
		if (key == PGZ) {
			auto it = PG.find(rec);
			if (it != PG.end()) {
				key -= 'Z' - AlphabetStart;
				key += 'i' - AlphabetStart;
				type = 'i';
				num = it->second;
			} else {
				int pos = PG.size();
				PG[rec] = pos;
			}
		} else if (key == RGZ) {
			auto it = RG.find(rec);
			if (it != RG.end()) {
				key -= 'Z' - AlphabetStart;
				key += 'i' - AlphabetStart;
				type = 'i';
				num = it->second;
			} else {
				int pos = RG.size();
				RG[rec] = pos;
			}
		} else if (key == MDZ) {
			if (!strcmp(rec, eo.MD.c_str())) {
				while (*rec) rec++; // Put rec to the end
			} else {
				DEBUG("MD calculation failed: calculated %s, found %s", eo.MD.c_str(), rec);
				failedMD++;
			}
		} else if (key == XDZ) {
			if (parseXD(rec, eo.MD)) {
				while (*rec) rec++; // Put rec to the end
			} else {
				// DEBUG("XD calculation failed: calculated %s, found %s", eoXD.c_str(), rec);
				failedXD++;
			}
		}

		if (type == 'i' && !(key == NMi && eo.NM != -1 && eo.NM == num)) { // number and not valid NMi
			int ret = packInteger(num, packedInt);
			key += ret + 1;
		}	
		
		int keyIndex = fields[key];
		if (keyIndex == -1) {
			keyIndex = fields[key] = fieldCount++;
			out.emplace_back(Array<uint8_t>(MB, MB));
			addEncoded(prevKey = keyIndex + 1, tags);
			addEncoded(key + 1, tags);
		} else {
			addEncoded(prevKey = keyIndex + 1, tags);
		}

		switch (type) {
		case 'i':
			if (key != NMi) out[keyIndex].add(packedInt.data(), packedInt.size());
			break;
		case 'd': // will round double to float; double is not supported by standard anyways
		case 'f': {
			float num = atof(rec);
			uint8_t *np = (uint8_t*)(&num);
			REPEAT(4) out[keyIndex].add(*np), np++;
			break;
		}
		case 'A':
			out[keyIndex].add(rec[0]); 
			break;
		default:
			while (*rec)
				out[keyIndex].add(*rec++);
			out[keyIndex].add(0);
			break;
		}
		while (*rec) rec++;
		packedInt.resize(0);
		size++;
	}
	ZAMAN_END(ParseFields);
	return size;
}
