#include "OptionalField.h"
#include "QualityScore.h"
#include "../Streams/rANSOrder2Stream.h"
using namespace std;

int totalXD, failedXD;
int totalMD, failedMD;

inline string keyStr(int f) 
{
	char c[4] = {0};
	c[0] = AlphabetStart + (f / AlphabetRange) / AlphabetRange;
	c[1] = AlphabetStart + (f / AlphabetRange) % AlphabetRange;
	c[2] = AlphabetStart + f % AlphabetRange;
	return string(c);
}

unordered_set<int> OptionalFieldCompressor::LibraryTags = {
	OptTagKey(PGZ),
	OptTagKey(RGZ),
	OptTagKey(LBZ),
	OptTagKey(PUZ)
};
unordered_set<int> OptionalFieldCompressor::QualityTags = {
	OptTagKey(BQZ),
	OptTagKey(CQZ),
	OptTagKey(E2Z),
	OptTagKey(OQZ),
	OptTagKey(QTZ),
	OptTagKey(Q2Z),
	OptTagKey(U2Z)
};

OptionalFieldCompressor::OptionalFieldCompressor(void):
	StringCompressor<GzipCompressionStream<6>>(),
	fields(AlphabetRange * AlphabetRange * AlphabetRange, -1),
	fieldCount(0)
{
}

OptionalFieldCompressor::~OptionalFieldCompressor (void) 
{
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
		LOG("  %-10s: %'20lu", keyStr(m.first).c_str(), m.second->getCount());
}

void OptionalFieldCompressor::outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k, const Array<OptionalField> &optFields, unordered_map<int32_t, map<string, int>> &library) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());
	ZAMAN_START(OptionalFieldOutput);

	Array<uint8_t> lib(MB, MB);
	for (auto &l: library) {
		lib.add((uint8_t*)&l.first, sizeof(uint32_t));
		int32_t len = l.second.size();
		lib.add((uint8_t*)&len, sizeof(uint32_t));
		for (auto &k: l.second) {
			lib.add((uint8_t*)k.first.c_str(), k.first.size() + 1);
			LOG("%d %s->%d", l.first, k.first.c_str(), k.second);
		}
	}

	vector<Array<uint8_t>> oa;
	Array<uint8_t> buffer(k * 10, MB);
	Array<uint8_t> sizes(k, MB);

	for (size_t i = 0; i < k; i++) {
		int size = processFields(records[i].getOptional(), 
			oa, buffer, optFields[i]);
		addEncoded(size + 1, sizes);
		totalSize -= records[i].getOptionalSize() + 1;
	}

	ZAMAN_START(Index);
	compressArray(streams[0], lib, out, out_offset);
	compressArray(streams[0], buffer, out, out_offset);
	compressArray(streams[0], sizes, out, out_offset);
	ZAMAN_END(Index);

	ZAMAN_START(Keys);

	vector<int> idxToKey(oa.size());
	for (int key = 0; key < fields.size(); key++) if (fields[key] != -1)
		idxToKey[fields[key]] = key;
	for (int keyIndex = 0; keyIndex < oa.size(); keyIndex++) {
		ZAMAN_START(C);
		int key = idxToKey[keyIndex];
		__zaman_prefix__ = __zaman_prefix__.substr(0, __zaman_prefix__.size() - 2) + keyStr(key);
		if (fieldStreams.find(key) == fieldStreams.end()) {
			if (QualityTags.find(key) != QualityTags.end()) {
				fieldStreams[key] = make_shared<rANSOrder2CompressionStream<128>>();
			} else {
				fieldStreams[key] = make_shared<GzipCompressionStream<6>>();
			}
		}
		compressArray(fieldStreams[key], oa[keyIndex], out, out_offset);
		ZAMAN_END(C);
		__zaman_prefix__ = __zaman_prefix__.substr(0, __zaman_prefix__.size() - 3 + 2);
	}
	ZAMAN_END(Keys);
	
	fill(fields.begin(), fields.end(), -1);
	fieldCount = 0;
	library.clear();
	ZAMAN_END(OptionalFieldOutput);
}

int OptionalFieldCompressor::processFields (const char *rec, vector<Array<uint8_t>> &out,
	 Array<uint8_t> &tags, const OptionalField &of)
{
	ZAMAN_START(ParseFields);
	int prevKey = 0;
	for (auto &kv: of.keys) {
		int key = kv.first;
		int keyIndex = fields[key];
		if (keyIndex == -1) {
			keyIndex = fields[key] = fieldCount++;
			out.emplace_back(Array<uint8_t>(MB, MB));
			addEncoded(prevKey = keyIndex + 1, tags);
			addEncoded(key + 1, tags);
		} else {
			addEncoded(prevKey = keyIndex + 1, tags);
		}

		int type = key % AlphabetRange + AlphabetStart;
		if (type >= 'i' || (type > 'Z' && type <= 'Z' + 5)) {
			packInteger(kv.second.I, out[keyIndex]);
		} else switch(type) {
		case 'd':
		case 'f': {
			uint8_t *np = (uint8_t*)(&kv.second.F);
			REPEAT(8) out[keyIndex].add(*np), np++;
			break;
		}
		case 'A':
			out[keyIndex].add(rec[kv.second.I]);
			break;
		default: 
			for (const char *r = rec + kv.second.I; *r; r++)
				out[keyIndex].add(*r);
			out[keyIndex].add(0);
		   break;
		}
	}
	ZAMAN_END(ParseFields);
	return of.keys.size();
}

OptionalField::OptionalField (const char *rec, const char *recEnd, unordered_map<int32_t, map<string, int>> &library) 
{
	ZAMAN_START(OptionalField);
	for (const char *recStart = rec; rec < recEnd; rec++) {
		if (recEnd - rec < 5 || rec[2] != ':' || rec[4] != ':') {
			LOG("ooops... %s %d %d", rec, rec[0], recEnd - rec);
			throw DZException("Invalid SAM tag %s", rec);
		}

		char type = rec[3];
		int key = OptTag(rec[0], rec[1], rec[3]);
		rec += 5;
		
		switch (type) {
		case 'i':
			keys.add(make_pair(key, IntFloatUnion((int64_t)atoi(rec))));
			break;
		case 'd': // will round double to float; double is not supported by standard anyways
		case 'f':
			keys.add(make_pair(key, IntFloatUnion((double)atof(rec))));
			break;
		default:
			keys.add(make_pair(key, IntFloatUnion((int64_t)(rec - recStart))));
			if (OptionalFieldCompressor::LibraryTags.find(key) != OptionalFieldCompressor::LibraryTags.end()) {
				library[key][rec] = 0;
			}

		}		
		while (*rec) rec++;
	}
	ZAMAN_END(OptionalField);
}

void OptionalField::parse(char *rec, const EditOperation &eo, unordered_map<int32_t, map<string, int>> &library)
{
	ZAMAN_START(OptionalField);
	for (auto &kv: keys) {
		int key = kv.first;
		char type = kv.first % AlphabetRange + AlphabetStart;

		// 1. set library tag to proper value
		// 2. check is MDZ/NM etc ok
		if (OptionalFieldCompressor::LibraryTags.find(key) != OptionalFieldCompressor::LibraryTags.end()) {
			assert(library[key].find(rec + kv.second.I) != library[key].end());
			type = 'i'; 
			kv.second.I = library[key][rec + kv.second.I];
		} else if (key == OptionalFieldCompressor::MDZ) {
			if (strcmp(rec + kv.second.I, eo.MD.c_str())) {
				DEBUG("MD calculation failed: calculated %s, found %s", eo.MD.c_str(), rec + kv.second.I);
				failedMD++;
			} else {
				rec[kv.second.I] = 0;
			}
			totalMD++;
		} else if (key == OptionalFieldCompressor::XDZ) {
			if (!parseXD(rec + kv.second.I, eo.MD)) {
				// DEBUG("XD calculation failed: calculated %s, found %s", eoXD.c_str(), rec + kv.second.I);
				failedXD++;
			} else {
				rec[kv.second.I] = 0;
			}
			totalXD++;
		} 
		if (type == 'i' && !(key == OptionalFieldCompressor::NMi && eo.NM != -1 && eo.NM == kv.second.I)) { // number and not valid NMi
			uint64_t num = kv.second.I, p = 0;
			while (num) num >>= 8, p++;
			if (!p) p = 1;
			if (p > 5) p = 5;
			kv.first += p;
		}	
	}
	ZAMAN_END(OptionalField);
}

string OptionalField::getXDfromMD(const string &eoMD)
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

bool OptionalField::parseXD(const char *rec, const string &eoMD)
{
	string eoXD = getXDfromMD(eoMD);
	for (int i = 0; i < eoXD.size(); i++)
		if (eoXD[i] != rec[i]) 
			return false;
	if (rec[eoXD.size()])
		return false;
	return true;
}
