#ifndef StringEngine_H
#define StringEngine_H

#include <string>
#include "../Common.h"
#include "GenericEngine.h"

template<typename TStream>
class StringCompressor: 
	public GenericCompressor<std::string, TStream> 
{
public:
	StringCompressor (int blockSize);
	virtual ~StringCompressor (void);

public:
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
	GenericCompressor<std::string, TStream>(blockSize) 
{
}

template<typename TStream>
StringCompressor<TStream>::~StringCompressor (void) {
}

template<typename TStream>
void StringCompressor<TStream>::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!this->records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= this->records.size());
	Array<uint8_t> buffer;
	for (size_t i = 0; i < k; i++)
		buffer.add((uint8_t*)this->records[i].c_str(), this->records[i].size() + 1);
	size_t s = this->stream->compress(buffer.data(), buffer.size(), out, out_offset);
	out.resize(out_offset + s);
	//// 
	this->records.remove_first_n(k);
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

	// here, we shouldn't have any leftovers, since all blocks are of the same size
	assert(this->recordCount == this->records.size());

	// decompress
	Array<uint8_t> au;
	// !TODO
	au.resize( 100000000 );
	size_t s = this->stream->decompress(in, in_size, au, 0);
	
	size_t start = 0;
	for (size_t i = 0; i < s; i++)
		if (au.data()[i] == 0) {
			this->records.add(std::string((char*)au.data() + start, i - start));
			start = i + 1;
		}
	
	this->recordCount = 0;
}

#endif
