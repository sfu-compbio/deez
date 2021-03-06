#ifndef GenericEngine_H
#define GenericEngine_H

#include "../Common.h"
#include "Engine.h"

template<typename T, typename TStream>
class GenericCompressor: public Compressor {
public:
	GenericCompressor(void);

public:
	virtual void outputRecords (const Array<Record> &records, Array<uint8_t> &out, size_t out_offset, size_t k) {};
	virtual void getIndexData (Array<uint8_t> &out);
};
 
template<typename T, typename TStream>
class GenericDecompressor: public Decompressor {
protected:
	Array<T> records;

public:
	GenericDecompressor (int blockSize);
	virtual ~GenericDecompressor (void);

public:
	const T &getRecord (size_t i);
	virtual T& operator[] (int idx);
	virtual size_t size() const { return records.size(); }

	virtual void importRecords (uint8_t *in, size_t in_size);
	virtual void setIndexData (uint8_t *in, size_t in_size);
};

template<typename T, typename TStream>
GenericCompressor<T, TStream>::GenericCompressor (void)
{
	streams.push_back(make_shared<TStream>());
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::getIndexData (Array<uint8_t> &out) 
{
	out.resize(0);
	streams.front()->getCurrentState(out);
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::GenericDecompressor (int blockSize): 
	records(blockSize, blockSize)
{
	streams.push_back(make_shared<TStream>());
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::~GenericDecompressor (void) 
{
}


template<typename T, typename TStream>
const T &GenericDecompressor<T, TStream>::getRecord (size_t i) 
{
	return records[i];
}

template<typename T, typename TStream>
T& GenericDecompressor<T, TStream>::operator[] (int idx) 
{
	return records[idx];
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) 
		return;
	assert(in_size >= sizeof(size_t));
	
	Array<uint8_t> out;
	size_t s = decompressArray(streams.front(), in, out);
	assert(s % sizeof(T) == 0);
	records.resize(0);
	records.add((T*)out.data(), s / sizeof(T));
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::setIndexData (uint8_t *in, size_t in_size) 
{
	streams.front()->setCurrentState(in, in_size);
}

#endif