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

public:
	virtual size_t size (void) const { return records.size(); }
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
	virtual bool hasRecord (void);
	virtual void importRecords (uint8_t *in, size_t in_size);
};

template<typename T, typename TStream>
GenericCompressor<T, TStream>::GenericCompressor (int blockSize):
	records(blockSize) 
{
	stream = new TStream();
}

template<typename T, typename TStream>
GenericCompressor<T, TStream>::~GenericCompressor (void) {
	delete stream;
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::addRecord (const T &rec) {
	records.add(rec);
}

template<typename T, typename TStream>
// set out size to compressed size block
void GenericCompressor<T, TStream>::outputRecords (Array<uint8_t> &out, size_t out_offset, size_t k) {
	if (!records.size()) { 
		out.resize(0);
		return;
	}
	assert(k <= records.size());
	Array<uint8_t> buffer;
	for (size_t i = 0; i < k; i++)
		buffer.add((uint8_t*)&records[i], sizeof(records[i]));
	size_t s = stream->compress(buffer.data(), buffer.size(), out, out_offset);
	out.resize(out_offset + s);
	//// 
	records.remove_first_n(k);
}

template<typename T, typename TStream>
GenericDecompressor<T, TStream>::GenericDecompressor (int blockSize): 
	records(blockSize), 
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

	// here, we shouldn't have any leftovers, since all blocks are of the same size
	assert(recordCount == records.size());

	// decompress
	Array<uint8_t> au;
	// !TODO
	au.resize( 100000000 );
	size_t s = stream->decompress(in, in_size, au, 0);
	assert(s % sizeof(T) == 0);
	records.add((T*)au.data(), s / sizeof(T));
	
	recordCount = 0;
}

#endif