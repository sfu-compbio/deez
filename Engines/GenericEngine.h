#ifndef GenericEngine_H
#define GenericEngine_H

#include "../Common.h"
#include "Engine.h"

template<typename T, typename TStream>
class GenericCompressor: public Compressor {
protected:
	CircularArray<T> records;

public:
	GenericCompressor (int blockSize);
	virtual ~GenericCompressor (void);

public:
	virtual void addRecord (const T &rec);
	virtual void outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k);
	virtual void getIndexData (Array<uint8_t> &out);
	virtual T& operator[] (int idx);

public:
	virtual size_t size (void) const { return records.size(); }
	virtual void resize(size_t sz) {
		records.resize(sz);
	}
	virtual size_t currentMemoryUsage() {
		return sizeInMemory(records) + sizeof(*this) - sizeof(records);
	}
};
 
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
	virtual T& operator[] (int idx);
	virtual size_t size() const { return records.size(); }

	virtual bool hasRecord (void);
	virtual void importRecords (uint8_t *in, size_t in_size);
	virtual void setIndexData (uint8_t *in, size_t in_size);
};

template<typename T, typename TStream>
GenericCompressor<T, TStream>::GenericCompressor (int blockSize):
	records(blockSize)
{
	streams.push_back(make_shared<TStream>());
	//records.resize(blockSize);
}

template<typename T, typename TStream>
GenericCompressor<T, TStream>::~GenericCompressor (void) 
{
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::addRecord (const T &rec) 
{
	records.add(rec);
}

template<typename T, typename TStream>
T& GenericCompressor<T, TStream>::operator[] (int idx) 
{
	assert(idx < records.size());
	return records[idx];
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) 
{
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());

	ZAMAN_START(Generic);
	Array<uint8_t> buffer(k * sizeof(T));
	for (size_t i = 0; i < k; i++)
		buffer.add((uint8_t*)&records[i], sizeof(T));
	compressArray(streams.front(), buffer, out, out_offset);
	records.remove_first_n(k);
	ZAMAN_END(Generic);
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::getIndexData (Array<uint8_t> &out) 
{
	out.resize(0);
	streams.front()->getCurrentState(out);
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::GenericDecompressor (int blockSize): 
	records(blockSize, blockSize), 
	recordCount(0)
{
	streams.push_back(make_shared<TStream>());
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::~GenericDecompressor (void) 
{
}

template<typename T, typename TStream>
bool GenericDecompressor<T, TStream>::hasRecord (void) 
{
	return recordCount < records.size();
}

template<typename T, typename TStream>
const T &GenericDecompressor<T, TStream>::getRecord (void) 
{
	assert(hasRecord());
	return records.data()[recordCount++];
}

template<typename T, typename TStream>
T& GenericDecompressor<T, TStream>::operator[] (int idx) 
{
	assert(idx < records.size());
	return records.data()[idx];
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::importRecords (uint8_t *in, size_t in_size) 
{
	if (in_size == 0) 
		return;

	//assert(recordCount == records.size());
	assert(in_size >= sizeof(size_t));
	
	Array<uint8_t> out;
	size_t s = decompressArray(streams.front(), in, out);
	assert(s % sizeof(T) == 0);
	records.resize(0);
	records.add((T*)out.data(), s / sizeof(T));
	recordCount = 0;
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::setIndexData (uint8_t *in, size_t in_size) 
{
	streams.front()->setCurrentState(in, in_size);
}

#endif