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

static FILE *fo;
OptionalFieldCompressor::OptionalFieldCompressor(void):
	StringCompressor<GzipCompressionStream<6>>(),
	fields(AlphabetRange * AlphabetRange * AlphabetRange, -1),
	prevIndex(AlphabetRange * AlphabetRange * AlphabetRange),
	fieldCount(0)
{
	streams.resize(Fields::ENUM_COUNT);
	for (int i = 0; i < streams.size(); i++)
		streams[i] = make_shared<GzipCompressionStream<6>>();
	streams[Fields::TAGX] = make_shared<rANSOrder0CompressionStream<256>>();
	streams[Fields::TAGLEN] = make_shared<rANSOrder0CompressionStream<256>>();
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
	LOG("  TagSig    : %'20lu", streams[0]->getCount());
	LOG("  TagX      : %'20lu", streams[1]->getCount());
	LOG("  TagLen    : %'20lu", streams[2]->getCount());
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
		//	 LOG("%d %s->%d", l.first, k.first.c_str(), k.second);
		}
	}

	vector<Array<uint8_t>> oa;
	Array<uint8_t> buffer(k * 10, MB);
	Array<uint8_t> buffer2(k * 10, MB);
	Array<uint8_t> sizes(k, MB);

	fill(prevIndex.begin(), prevIndex.end(), 0);
	unordered_map<int, int> sizeMap;
	for (size_t i = 0; i < k; i++) {
		int size = processFields(records[i].getOptional(), 
			oa, buffer, buffer2, optFields[i], k);
		auto it = sizeMap.find(size);
		if (it != sizeMap.end()) {
			addEncoded(it->second, sizes);
		} else {
			int p = sizeMap.size() + 1;
			sizeMap[size] = p;
			addEncoded(p, sizes);
			addEncoded(size + 1, sizes);
		}
		addEncoded(1, buffer);
		totalSize -= records[i].getOptionalSize() + 1;
	}

	ZAMAN_START(Index);
	compressArray(streams[Fields::TAG], lib, out, out_offset);
	compressArray(streams[Fields::TAG], buffer, out, out_offset);
	//compressArray(streams[Fields::TAGX], buffer2, out, out_offset);
	//compressArray(streams[Fields::TAGLEN], sizes, out, out_offset);
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
	 Array<uint8_t> &tags, Array<uint8_t> &tags2, const OptionalField &of, size_t k)
{
	ZAMAN_START(ParseFields);
	int pi = -1, pk = 0;
	for (auto &kv: of.keys) {
		int key = kv.first;

		int type = key % AlphabetRange + AlphabetStart;
		int32_t keyIndex = fields[key];

		if (keyIndex == -1) {
			keyIndex = fields[key] = fieldCount++;
			if (type >= 'd') {
				out.emplace_back(Array<uint8_t>(8 * k, MB));
			} else {
				int l = strlen(rec + kv.second);
				out.emplace_back(Array<uint8_t>((l + 1) * k, MB));
			}
			addEncoded(keyIndex - pi + 1, tags);
			addEncoded(key + 1, tags);
		} else {
			addEncoded(keyIndex - pi + 1, tags);
		}
		if (keyIndex < pi) {
			swap(fields[key], fields[pk]);
			swap(out[keyIndex], out[pi]);
		}
		printf("%d ", keyIndex - pi + 1);
		pi = keyIndex;
		pk = key;


		if (key == OptionalFieldCompressor::NMi) // it was calculated
			continue;

		// only zero is j!!!
		int cnt;
		if (type <= 'Z') { // strings
			cnt = strlen(rec + kv.second);
			if (type > 'A') cnt++; // include 0
		} else if (type < 'i') { // d, f, library
			cnt = 4;
		} else {
			cnt = type - 'i';
		}
		out[keyIndex].add((uint8_t*)(rec + kv.second), cnt);
	}
	addEncoded(1, tags2);
	printf("\n");
	ZAMAN_END(ParseFields);
	return of.keys.size();
}

void OptionalField::parse1 (const char *rec, const char *recEnd, unordered_map<int32_t, map<string, int>> &library)
{
	ZAMAN_START(OptionalField);
	keys.resize(0);
	for (const char *recStart = rec; rec < recEnd; rec++) {
		if (recEnd - rec < 5 || rec[2] != ':' || rec[4] != ':') {
			LOG("ooops... %s %d %d", rec, rec[0], recEnd - rec);
			throw DZException("Invalid SAM tag %s", rec);
		}

		char type = rec[3];
		int key = OptTag(rec[0], rec[1], rec[3]);
		rec += 5;
		switch (type) {
		case 'i': {
			int64_t num = atoi(rec), sz = 0;
			if (num >= numeric_limits<int8_t>::min() && num <= numeric_limits<int8_t>::max()) {
				uint8_t n = num;
				memcpy((uint8_t*)rec, &n, sz = sizeof(uint8_t));
			} else if (num >= numeric_limits<int16_t>::min() && num <= numeric_limits<int16_t>::max()) {
				uint16_t n = num;
				memcpy((uint8_t*)rec, &n, sz = sizeof(uint16_t));
			} else if (num >= numeric_limits<int32_t>::min() && num <= numeric_limits<int32_t>::max()) {
				uint32_t n = num;
				memcpy((uint8_t*)rec, &n, sz = sizeof(uint32_t));
			} else {
				uint64_t n = num;
				memcpy((uint8_t*)rec, &n, sz = sizeof(uint64_t));
			}
			keys.add(make_pair(key + sz, rec - recStart));
			rec += sz;
			break;
		}
		case 'd': // will round double to float; double is not supported by standard anyways
		case 'f': {
			float n = atof(rec);
			memcpy((uint8_t*)(rec - 4), &n, sizeof(float)); // to make sure it does fit!
			keys.add(make_pair(key, rec - 4 - recStart));
			break;
		}
		default: {
			keys.add(make_pair(key, rec - recStart));
			if (OptionalFieldCompressor::LibraryTags.find(key) != OptionalFieldCompressor::LibraryTags.end()) 
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
			assert(library[key].find(rec + kv.second) != library[key].end());
			// 4 bita!!!!!!!!!!
			// overwrite PG:x:
			int32_t n = library[key][rec + kv.second];
			kv.second -= 4;
			memcpy(rec + kv.second, &n, sizeof(uint32_t));
			kv.first += 4;
		} else if (key == OptionalFieldCompressor::MDZ) {
			if (strcmp(rec + kv.second, eo.MD.c_str())) {
				DEBUG("MD calculation failed: calculated %s, found %s", eo.MD.c_str(), rec + kv.second);
				failedMD++;
			} else {
				rec[kv.second] = 0;
			}
			totalMD++;
		} else if (key == OptionalFieldCompressor::XDZ) {
			if (!parseXD(rec + kv.second, eo.MD)) {
				// DEBUG("XD calculation failed: calculated %s, found %s", eoXD.c_str(), rec + kv.second.I);
				failedXD++;
			} else {
				rec[kv.second] = 0;
			}
			totalXD++;
		} else if (key >= OptionalFieldCompressor::NMi && key <= OptionalFieldCompressor::NMi + 8 && eo.NM != -1) {
			int64_t NM;
			if (type == 'i' + 1) NM = *(uint8_t*)(rec + kv.second);
			else if (type == 'i' + 2) NM = *(uint16_t*)(rec + kv.second);
			else if (type == 'i' + 4) NM = *(uint32_t*)(rec + kv.second);
			else NM = *(uint64_t*)(rec + kv.second);
			if (NM == eo.NM)
				kv.first = OptionalFieldCompressor::NMi;
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
