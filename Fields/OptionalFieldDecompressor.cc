#include "OptionalField.h"
#include "../Streams/rANSOrder2Stream.h"
using namespace std;

OptionalFieldDecompressor::OptionalFieldDecompressor (int blockSize):
	GenericDecompressor<OptionalField, GzipDecompressionStream>(blockSize),
	fields(AlphabetRange * AlphabetRange * AlphabetRange, -1)
{
}

OptionalFieldDecompressor::~OptionalFieldDecompressor (void) 
{
}

void OptionalFieldDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	ZAMAN_START(ImportOptionalField);

	Array<uint8_t> index, sizes, lib;
	size_t l = decompressArray(streams[0], in, lib);
	library.clear();
	for (uint8_t *i = lib.data(); i < lib.data() + lib.size();) {
		uint32_t key = *(uint32_t*)i; i += sizeof(uint32_t);
		uint32_t len = *(uint32_t*)i; i += sizeof(uint32_t);
		for (int j = 0; j < len; j++) {
			string s;
			while (*i) s += *i++; i++;
			library[key][j] = s;

			LOG("%d %d->%s", key, j, s.c_str());
		}
	}

	size_t s = decompressArray(streams[0], in, index);
	decompressArray(streams[0], in, sizes);
	uint8_t *tags = index.data();
	uint8_t *szs = sizes.data();

	vector<Array<uint8_t>> oa;
	vector<size_t> out;

	fill(fields.begin(), fields.end(), -1);

	records.resize(0);
	while (szs != sizes.data() + sizes.size()) {
		int size = getEncoded(szs) - 1;
		records.add(parseFields(size, tags, in, oa, out));
	}

	recordCount = 0;
	ZAMAN_END(ImportOptionalField);
}

OptionalField OptionalFieldDecompressor::parseFields(int size, uint8_t *&tags, uint8_t *&in, 
	vector<Array<uint8_t>>& oa, vector<size_t> &out)
{
	ZAMAN_START(ParseFields);

	OptionalField of;
	string &result = of.data;
	while (size--) {
		int keyIndex = getEncoded(tags) - 1;
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

		if (result != "") result += "\t";
		result += S("%c%c:%c:", 
			AlphabetStart + (key / AlphabetRange) / AlphabetRange,
			AlphabetStart + (key / AlphabetRange) % AlphabetRange,
			AlphabetStart + key % AlphabetRange);

		int keyZ = key - (key % AlphabetRange) + ('Z' - AlphabetStart);
		int type = key % AlphabetRange + AlphabetStart;
		if (type > 'Z' && type <= 'Z' + 5) {
			assert(OptionalFieldCompressor::LibraryTags.find(keyZ) != OptionalFieldCompressor::LibraryTags.end());
			int64_t num = unpackInteger(key % AlphabetRange - ('Z' - AlphabetStart) - 1, oa[keyIndex], out[keyIndex]);			
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
			case 'i' + 1:
			case 'i' + 2:
			case 'i' + 3:
			case 'i' + 4:
			case 'i' + 5: {
				result[result.size() - 2] = 'i'; // SAM only supports 'i' as tag
				int64_t num = unpackInteger(type - 'i' - 1, oa[keyIndex], out[keyIndex]);
				result += inttostr(num);
				break;
			}
			case 'd':
			case 'f': {
				uint64_t num = 0;
				REPEAT(8) num |= uint64_t(oa[keyIndex].data()[out[keyIndex]++]) << (8 * _);
				result += S("%g", *((double*)(&num)));
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
