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
	//streams[OptionalFieldCompressor::Fields::TAG] = make_shared<rANSOrder0DecompressionStream<256>>();
}

OptionalFieldDecompressor::~OptionalFieldDecompressor (void) 
{
}

void OptionalFieldDecompressor::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) return;

	ZAMAN_START(OptionalField);

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
			// LOG("%d %d->%s", key, j, s.c_str());
		}
	}

	decompressArray(streams[OptionalFieldCompressor::Fields::TAG], in, index);
	uint8_t *tags = index.data();
	
	data.clear();
	dataLoc.clear();
	positions.clear();

	fill(fields.begin(), fields.end(), -1);
	fill(prevIndex.begin(), prevIndex.end(), 0);

	records.resize(0);
	inputBuffer = in;
	while (tags != index.data() + index.size()) {
		records.add(parseFields(tags, in));
	}
	condition.notify_all();

	ZAMAN_END(OptionalField);
}

inline OptionalField OptionalFieldDecompressor::parseFields(uint8_t *&tags, uint8_t *&in)
{
	OptionalField of;
	for (int size = 0, pi = -1; ; size++) {
		int f = getEncoded(tags);
		if (f - 1 == 0)
			break;
		if (f > 1)
			f--;
		int keyIndex = pi + f;
		int key = fields[keyIndex];
		if (key == -1) {
			key = getEncoded(tags) - 1;
			fields[keyIndex] = key;
			if (fieldStreams.find(key) == fieldStreams.end()) {
				if (OptionalFieldCompressor::QualityTags.find(key) != OptionalFieldCompressor::QualityTags.end()) {
					fieldStreams[key] = make_shared<rANSOrder2DecompressionStream<128>>();
				} else {
					fieldStreams[key] = make_shared<GzipDecompressionStream>();
				}
			}
			assert(data.size() == keyIndex);
			data.push_back(Array<uint8_t>());
			dataLoc.push_back(Array<size_t>(records.capacity(), records.capacity()));
			positions.push_back(0);
		}
		prevIndex[size] = key;
	
		assert(key != -1);
		of.keys.add(make_pair(keyIndex, positions[keyIndex]++));
	}
	return of;
}

void OptionalFieldDecompressor::decompressThreads(ctpl::thread_pool &pool)
{
	unique_lock<std::mutex> lock(conditionMutex);
    condition.wait(lock);

	for (int i = 0; i < data.size(); i++) {
		pool.push([&](int T, int ti, int key, shared_ptr<DecompressionStream> stream, uint8_t *in) {
			ZAMAN_START(OptionalField_C);
			#ifdef ZAMAN
				__zaman_thread__.prefix = __zaman_thread__.prefix.substr(0, __zaman_thread__.prefix.size() - 2) + keyStr(key);
			#endif

			decompressArray(stream, in, data[ti]);
			int type = key % AlphabetRange + AlphabetStart;
			if (type > 'A' && type <= 'Z') {
				dataLoc[ti].add(0);
				for (size_t i = 0, j = 0; i < data[ti].size(); i++) {
					if (data[ti][i] == 0) {
						dataLoc[ti].add(i + 1);
					}
				}
			}
			ZAMAN_END(OptionalField_C);
			#ifdef ZAMAN
				__zaman_thread__.prefix = __zaman_thread__.prefix.substr(0, __zaman_thread__.prefix.size() - 3 + 2);
			#endif
			ZAMAN_THREAD_JOIN();
		}, i, fields[i], fieldStreams[fields[i]], inputBuffer);
		
		size_t in_sz = *(size_t*)inputBuffer;
		inputBuffer += 2 * sizeof(size_t) + in_sz;
	}
}

void OptionalFieldDecompressor::getRecord(size_t record, const EditOperation &eo, string &result) 
{
	ZAMAN_START(OptionalFieldGet);
	OptionalField &of = records[record];

	for (auto &kv: of.keys) {
		int keyIndex = kv.first;
		int key = fields[keyIndex];
		int i = kv.second;

		result += "\t  : :";
		result[result.size() - 5] = AlphabetStart + (key / AlphabetRange) / AlphabetRange;
		result[result.size() - 4] = AlphabetStart + (key / AlphabetRange) % AlphabetRange;
		result[result.size() - 2] = AlphabetStart + key % AlphabetRange;

		int type = key % AlphabetRange + AlphabetStart;
		if (type == 'Z' + 4) {
			assert(OptionalFieldCompressor::LibraryTags.find(key - 4) != OptionalFieldCompressor::LibraryTags.end());
			uint32_t num = *(uint32_t*)(data[keyIndex].data() + i * sizeof(uint32_t));
			result[result.size() - 2] = 'Z';
			assert(library[key - 4].find(num) != library[key - 4].end());
			result += library[key - 4][num];
		} else if (key == OptionalFieldCompressor::MDZ && !data[keyIndex][dataLoc[keyIndex][i]]) {
			result += eo.MD;
		} else if (key == OptionalFieldCompressor::XDZ && !data[keyIndex][dataLoc[keyIndex][i]]) {
			result += OptionalField::getXDfromMD(eo.MD);
		} else if (key == OptionalFieldCompressor::NMi) {
			inttostr(eo.NM, result);
		} else switch (type) {
			case 'i' + 1: {
				int8_t num = *(uint8_t*)(data[keyIndex].data() + i * sizeof(int8_t));
				result[result.size() - 2] = 'i'; 
				inttostr(num, result);
				break;
			}
			case 'i' + 2: {
				int16_t num = *(uint16_t*)(data[keyIndex].data() + i * sizeof(int16_t));
				result[result.size() - 2] = 'i'; 
				inttostr(num, result);
				break;
			}
			case 'i' + 4: {
				int32_t num = *(uint32_t*)(data[keyIndex].data() + i * sizeof(int32_t));
				result[result.size() - 2] = 'i'; 
				inttostr(num, result);
				break;
			}
			case 'i' + 8: {
				int64_t num = *(uint64_t*)(data[keyIndex].data() + i * sizeof(int64_t));
				result[result.size() - 2] = 'i'; 
				inttostr(num, result);
				break;
			}
			case 'd':
			case 'f': {
				uint32_t num = 0;
				int f = i * 4;
				REPEAT(4) num |= uint32_t(data[keyIndex].data()[f++]) << (8 * _);
				result += S("%g", *((float*)(&num)));
				break;
			}
			case 'A':
				result += (char)(data[keyIndex].data()[i]);
				break;
			default: {
				int k = dataLoc[keyIndex][i], p = 0;
				while (data[keyIndex].data()[k])
					result += data[keyIndex].data()[k++], p++;
				if (OptionalFieldCompressor::LibraryTags.find(key - 4) != OptionalFieldCompressor::LibraryTags.end()) {
					int pos = library[key - 4].size();
					library[key - 4][pos] = result.substr(result.size() - p);
				}
				break;
			}
		}
	}

	ZAMAN_END(OptionalFieldGet);
}
