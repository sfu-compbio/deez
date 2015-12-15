#ifndef StringEngine_H
#define StringEngine_H

#include <string>
#include "../Common.h"
#include "GenericEngine.h"

template<typename TStream>
class StringCompressor: 
	public GenericCompressor<std::string, TStream> 
{
protected:
	size_t totalSize;

public:
	StringCompressor (int blockSize);
	virtual ~StringCompressor (void);

public:
	virtual void addRecord (const std::string &rec);
	virtual void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
};
 
template<typename TStream>
class StringDecompressor: 
	public GenericDecompressor<std::string, TStream> 
{
public:
	StringDecompressor (int blockSize);
	virtual ~StringDecompressor (void);

public:
	virtual void importRecords (uint8_t *in, size_t in_size);
};

template<typename TStream>
StringCompressor<TStream>::StringCompressor (int blockSize):
	GenericCompressor<std::string, TStream>(blockSize),
	totalSize(0)
{
}

template<typename TStream>
StringCompressor<TStream>::~StringCompressor (void) {
}

template<typename TStream>
void StringCompressor<TStream>::addRecord (const std::string &rec) {
	GenericCompressor<std::string, TStream>::addRecord(rec);
	totalSize += rec.size() + 1;
}

template<typename TStream>
void StringCompressor<TStream>::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!this->records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= this->records.size());

	ZAMAN_START(Compress_Strings);
	Array<uint8_t> buffer(totalSize);
	
	for (size_t i = 0; i < k; i++) {
		buffer.add((uint8_t*)this->records[i].c_str(), this->records[i].size() + 1);
		totalSize -= this->records[i].size() + 1;
	}
	
	compressArray(this->stream, buffer, out, out_offset);
	this->records.remove_first_n(k);
	ZAMAN_END(Compress_Strings);
}

template<typename TStream>
StringDecompressor<TStream>::StringDecompressor (int blockSize): 
	GenericDecompressor<std::string, TStream>(blockSize)
{
}

template<typename TStream>
StringDecompressor<TStream>::~StringDecompressor (void) {
}

template<typename TStream>
void StringDecompressor<TStream>::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) 
		return;

	//assert(this->recordCount == this->records.size());
	assert(in_size >= sizeof(size_t));
	
	Array<uint8_t> out;
	size_t s = decompressArray(this->stream, in, out);
	size_t start = 0;

	this->records.resize(0);
	for (size_t i = 0; i < s; i++)
		if (out.data()[i] == 0) {
			this->records.add(std::string((char*)out.data() + start, i - start));
			start = i + 1;
		}	
	this->recordCount = 0;
}

#endif
