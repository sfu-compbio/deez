#include "OptionalField.h"
using namespace std;

OptionalFieldDecompressor::OptionalFieldDecompressor (int blockSize):
	GenericDecompressor<OptionalField, GzipDecompressionStream>(blockSize),
	fields(AlphabetRange * AlphabetRange * AlphabetRange, -1)
{
}

OptionalFieldDecompressor::~OptionalFieldDecompressor (void) 
{
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
			if (fieldStreams.find(key) == fieldStreams.end())
				fieldStreams[key] = make_shared<GzipDecompressionStream>();
			oa.push_back(Array<uint8_t>());
			decompressArray(fieldStreams[key], in, oa[keyIndex]);
			out.push_back(0);
		}

		if (result != "") result += "\t";
		result += S("%c%c:%c:", 
			AlphabetStart + (key / AlphabetRange) / AlphabetRange,
			AlphabetStart + (key / AlphabetRange) % AlphabetRange,
			AlphabetStart + key % AlphabetRange);

		if (key > PGi && key <= PGi + 5) {
			int64_t num = unpackInteger(key % AlphabetRange - 'i' + AlphabetStart - 1, oa[keyIndex], out[keyIndex]);
			result[result.size() - 2] = 'Z';
			assert(PG.find(num) != PG.end());
			result += PG[num];	
		} else if (key > RGi && key <= RGi + 5) {
			int64_t num = unpackInteger(key % AlphabetRange - 'i' + AlphabetStart - 1, oa[keyIndex], out[keyIndex]);
			result[result.size() - 2] = 'Z';
			assert(RG.find(num) != RG.end());
			result += RG[num];	
		} else if (key == MDZ && !oa[keyIndex][out[keyIndex]]) {
			of.posMD = result.size();
			out[keyIndex]++;
		} else if (key == XDZ && !oa[keyIndex][out[keyIndex]]) {
			of.posXD = result.size();
			out[keyIndex]++;
		} else if (key == NMi) {
			of.posNM = result.size();
		} else switch (key % AlphabetRange + AlphabetStart) {
			case 'i' + 1:
			case 'i' + 2:
			case 'i' + 3:
			case 'i' + 4:
			case 'i' + 5: {
				result[result.size() - 2] = 'i'; // SAM only supports 'i' as tag
				int64_t num = unpackInteger(key % AlphabetRange - 'i' + AlphabetStart - 1, oa[keyIndex], out[keyIndex]);
				result += inttostr(num);
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
				int p = 0;
				while (oa[keyIndex].data()[out[keyIndex]])
					result += oa[keyIndex].data()[out[keyIndex]++], p++;
				if (key == PGZ) {
					int pos = PG.size();
					PG[pos] = result.substr(result.size() - p);
				}
				if (key == RGZ) {
					int pos = RG.size();
					RG[pos] = result.substr(result.size() - p);
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
		string XD = OptionalFieldCompressor::getXDfromMD(eo.MD);
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

void OptionalFieldDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	ZAMAN_START(ImportOptionalField);

	Array<uint8_t> index, sizes;
	size_t s = decompressArray(streams[0], in, index);
	decompressArray(streams[0], in, sizes);
	uint8_t *tags = index.data();
	uint8_t *szs = sizes.data();

	vector<Array<uint8_t>> oa;
	vector<size_t> out;

	PG.clear();
	RG.clear();
	fill(fields.begin(), fields.end(), -1);

	records.resize(0);
	while (szs != sizes.data() + sizes.size()) {
		int size = getEncoded(szs) - 1;
		records.add(parseFields(size, tags, in, oa, out));
	}

	recordCount = 0;
	ZAMAN_END(ImportOptionalField);
}
