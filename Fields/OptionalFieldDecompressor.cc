#include "OptionalField.h"
#include "../Streams/rANSOrder0Stream.h"
#include "../Streams/rANSOrder2Stream.h"
using namespace std;

OptionalFieldDecompressor::OptionalFieldDecompressor (int blockSize):
	GenericDecompressor<OptionalField, GzipDecompressionStream>(blockSize),
	fields(AlphabetRange * AlphabetRange * AlphabetRange, -1),
	prevIndex(AlphabetRange * AlphabetRange * AlphabetRange)
{
	streams.resize(OptionalFieldCompressor::Fields::ENUM_COUNT);
	for (int i = 0; i < streams.size(); i++)
		streams[i] = make_shared<GzipDecompressionStream>();
	streams[OptionalFieldCompressor::Fields::TAGX] = make_shared<rANSOrder0DecompressionStream<256>>();
	streams[OptionalFieldCompressor::Fields::TAGLEN] = make_shared<rANSOrder0DecompressionStream<256>>();
}

OptionalFieldDecompressor::~OptionalFieldDecompressor (void) 
{
}

void OptionalFieldDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	ZAMAN_START(ImportOptionalField);

	Array<uint8_t> index, index2, sizes, lib;
	size_t l = decompressArray(streams[OptionalFieldCompressor::Fields::TAG], in, lib);
	library.clear();
	for (uint8_t *i = lib.data(); i < lib.data() + lib.size();) {
		uint32_t key = *(uint32_t*)i; i += sizeof(uint32_t);
		uint32_t len = *(uint32_t*)i; i += sizeof(uint32_t);
		for (int j = 0; j < len; j++) {
			string s;
			while (*i) s += *i++; i++;
			library[key][j] = s;
		//	 LOG("%d %d->%s", key, j, s.c_str());
		}
	}

// 	  100.0% [xx,1]255 1
// 510 2
// 1 3

	size_t s = decompressArray(streams[OptionalFieldCompressor::Fields::TAG], in, index);
	decompressArray(streams[OptionalFieldCompressor::Fields::TAGX], in, index2);
	decompressArray(streams[OptionalFieldCompressor::Fields::TAGLEN], in, sizes);
	uint8_t *tags = index.data();
	uint8_t *tags2 = index2.data();
	uint8_t *szs = sizes.data();

	vector<Array<uint8_t>> oa;
	vector<size_t> out;

	fill(fields.begin(), fields.end(), -1);
	fill(prevIndex.begin(), prevIndex.end(), 0);

	records.resize(0);
	unordered_map<int, int> sizeMap;
	while (szs != sizes.data() + sizes.size()) {
		//if (szs) szs += y;
		int size = getEncoded(szs);
		auto it = sizeMap.find(size);
		if (it != sizeMap.end()) {
			size = sizeMap[size];
		} else {
			assert(size - 1 == sizeMap.size());
			int sz = getEncoded(szs) - 1;
			sizeMap[size] = sz;
			size = sz;
		}
		records.add(parseFields(size, tags, tags2, in, oa, out));
	}

	recordCount = 0;
	ZAMAN_END(ImportOptionalField);
}

OptionalField OptionalFieldDecompressor::parseFields(int size, uint8_t *&tags, uint8_t *&tags2, uint8_t *&in, 
	vector<Array<uint8_t>>& oa, vector<size_t> &out)
{
	ZAMAN_START(ParseFields);

	OptionalField of;
	string &result = of.data;

	int pi = -1;
	while (true) {
		int f = getEncoded(tags2) - 1;
		if (!f) break;
		int i = pi + f;
		int keyIndex = getEncoded(tags) - 1;
		prevIndex[i] = keyIndex;
		pi = i;

		int key = fields[keyIndex];
		if (key == -1) {
			key = getEncoded(tags) - 1;
			fields[keyIndex] = key;
			assert(oa.size() == keyIndex);
			assert(out.size() == keyIndex);
			if (fieldStreams.find(key) == fieldStreams.end()) {
				if (OptionalFieldCompressor::QualityTags.find(key) != OptionalFieldCompressor::QualityTags.end()) {
					fieldStreams[key] = make_shared<rANSOrder2DecompressionStream<128>>();
				} else {
					fieldStreams[key] = make_shared<GzipDecompressionStream>();
				}
			}
			oa.push_back(Array<uint8_t>());
			decompressArray(fieldStreams[key], in, oa[keyIndex]);
			out.push_back(0);
		}
	}

	for (int fi = 0; fi < size; fi++) {
		int keyIndex = prevIndex[fi];
		int key = fields[keyIndex];
		assert(key != -1);

		if (result != "") result += "\t";
		result += S("%c%c:%c:", 
			AlphabetStart + (key / AlphabetRange) / AlphabetRange,
			AlphabetStart + (key / AlphabetRange) % AlphabetRange,
			AlphabetStart + key % AlphabetRange);

		int keyZ = key - (key % AlphabetRange) + ('Z' - AlphabetStart);
		int type = key % AlphabetRange + AlphabetStart;
		if (type == 'Z' + 4) {
			assert(OptionalFieldCompressor::LibraryTags.find(keyZ) != OptionalFieldCompressor::LibraryTags.end());
			uint32_t num = *(uint32_t*)(oa[keyIndex].data() + out[keyIndex]);
			out[keyIndex] += sizeof(uint32_t);
			result[result.size() - 2] = 'Z';
			assert(library[keyZ].find(num) != library[keyZ].end());
			result += library[keyZ][num];
		} else if (key == OptionalFieldCompressor::MDZ && !oa[keyIndex][out[keyIndex]]) {
			of.posMD = result.size();
			out[keyIndex]++;
		} else if (key == OptionalFieldCompressor::XDZ && !oa[keyIndex][out[keyIndex]]) {
			of.posXD = result.size();
			out[keyIndex]++;
		} else if (key == OptionalFieldCompressor::NMi) {
			of.posNM = result.size();
		} else switch (type) {
			/*case 'Z' + 1:
			case 'Z' + 2:
			case 'Z' + 4: {
				assert(OptionalFieldCompressor::LibraryTags.find(keyZ) != OptionalFieldCompressor::LibraryTags.end());
				uint32_t num = *(uint32_t)(oa[keyIndex].data() + out[keyIndex]);
				out[keyIndex] += sizeof(uint32_t);
				result[result.size() - 2] = 'Z';
				assert(library[keyZ].find(num) != library[keyZ].end());
				result += library[keyZ][num];
			}*/
			case 'i' + 1: {
				int8_t num = *(uint8_t*)(oa[keyIndex].data() + out[keyIndex]);
				result[result.size() - 2] = 'i'; 
				result += inttostr(num);
				out[keyIndex] += sizeof(num);
				break;
			}
			case 'i' + 2: {
				int16_t num = *(uint16_t*)(oa[keyIndex].data() + out[keyIndex]);
				result[result.size() - 2] = 'i'; 
				result += inttostr(num);
				out[keyIndex] += sizeof(num);
				break;
			}
			case 'i' + 4: {
				int32_t num = *(uint32_t*)(oa[keyIndex].data() + out[keyIndex]);
				result[result.size() - 2] = 'i'; 
				result += inttostr(num);
				out[keyIndex] += sizeof(num);
				break;
			}
			case 'i' + 8: {
				int64_t num = *(uint64_t*)(oa[keyIndex].data() + out[keyIndex]);
				result[result.size() - 2] = 'i'; 
				result += inttostr(num);
				out[keyIndex] += sizeof(num);
				break;
			}
			case 'd':
			case 'f': {
				uint32_t num = 0;
				REPEAT(4) num |= uint32_t(oa[keyIndex].data()[out[keyIndex]++]) << (8 * _);
				result += S("%g", *((float*)(&num)));
				break;
			}
			case 'A':
				result += (char)(oa[keyIndex].data()[out[keyIndex]++]);
				break;
			default: {
				int p = 0;
				while (oa[keyIndex].data()[out[keyIndex]])
					result += oa[keyIndex].data()[out[keyIndex]++], p++;
				if (OptionalFieldCompressor::LibraryTags.find(keyZ) != OptionalFieldCompressor::LibraryTags.end()) {
					int pos = library[keyZ].size();
					library[keyZ][pos] = result.substr(result.size() - p);
				}
				out[keyIndex]++;
				break;
			}
		}
	}

	ZAMAN_END(ParseFields);
	return of;
}

const OptionalField &OptionalFieldDecompressor::getRecord(const EditOperation &eo) 
{
	assert(hasRecord());
	ZAMAN_START(GetOptionalField);
	OptionalField &of = records.data()[recordCount++];

	if (of.posMD != -1) {
		of.data = of.data.substr(0, of.posMD) + eo.MD + of.data.substr(of.posMD);
		if (of.posNM > of.posMD) of.posNM += eo.MD.size();
		if (of.posXD > of.posMD) of.posXD += eo.MD.size();
	}
	if (of.posXD != -1) {
		string XD = OptionalField::getXDfromMD(eo.MD);
		of.data = of.data.substr(0, of.posXD) + XD + of.data.substr(of.posXD);
		if (of.posNM > of.posXD) of.posNM += XD.size();
	}
	if (of.posNM != -1) {
		//LOG("%d %d %s %d", of.posNM, eo.NM, of.data.c_str(), of.data.size());
		of.data = of.data.substr(0, of.posNM) + inttostr(eo.NM) + of.data.substr(of.posNM);
	}
	ZAMAN_END(GetOptionalField);
	return of;
}
