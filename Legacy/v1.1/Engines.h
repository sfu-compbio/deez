#pragma once

#include "Common.h"
#include "Streams.h"

namespace Legacy {
namespace v11 {

/****************************************************************************************/

class Decompressor {
protected:
	DecompressionStream *stream;

public:
	virtual ~Decompressor (void) {}

public:
	virtual bool hasRecord (void) = 0;
	virtual void importRecords (uint8_t *in, size_t in_size) = 0;
	virtual void setIndexData (uint8_t *, size_t) = 0;
};

inline size_t decompressArray (DecompressionStream *d, uint8_t* &in, Array<uint8_t> &out) 
{
	size_t in_sz = *(size_t*)in; in += sizeof(size_t);
	size_t de_sz = *(size_t*)in; in += sizeof(size_t);
	
	out.resize(de_sz);
	size_t sz = 0;
	if (in_sz) 
		sz = d->decompress(in, in_sz, out, 0);
	assert(sz == de_sz);
	in += in_sz;
	return sz;
}

/****************************************************************************************/
 
template<typename T, typename TStream>
class GenericDecompressor: public Decompressor {
protected:
	Array<T> records;
	size_t recordCount;

public:
	GenericDecompressor (int blockSize);
	virtual ~GenericDecompressor (void);

public:
	const T &getRecord (void);
	virtual bool hasRecord (void);
	virtual void importRecords (uint8_t *in, size_t in_size);
	virtual void setIndexData (uint8_t *in, size_t in_size);
};

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::GenericDecompressor (int blockSize): 
	records(blockSize, blockSize), 
	recordCount (0) 
{
	stream = new TStream();
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::~GenericDecompressor (void) {
	delete stream;
}

template<typename T, typename TStream>
bool GenericDecompressor<T, TStream>::hasRecord (void) {
	return recordCount < records.size();
}

template<typename T, typename TStream>
const T &GenericDecompressor<T, TStream>::getRecord (void) {
	assert(hasRecord());
	return records.data()[recordCount++];
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::importRecords (uint8_t *in, size_t in_size) {
	if (in_size == 0) 
		return;

	//assert(recordCount == records.size());
	assert(in_size >= sizeof(size_t));
	
	Array<uint8_t> out;
	size_t s = decompressArray(stream, in, out);
	assert(s % sizeof(T) == 0);
	records.resize(0);
	records.add((T*)out.data(), s / sizeof(T));
	recordCount = 0;
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::setIndexData (uint8_t *in, size_t in_size) {
	stream->setCurrentState(in, in_size);
}

/****************************************************************************************/

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

};
};
