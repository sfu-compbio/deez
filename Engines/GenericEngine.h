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
	virtual void setIndexData (uint8_t *in, size_t in_size);
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
	Array<uint8_t> buffer(k * sizeof(T));
	
	//T *it = records.head();
	for (size_t i = 0; i < k; i++)
		buffer.add((uint8_t*)&records[i], sizeof(T));
	/*for (size_t i = 0; i < k; i++, it = records.increase(it))
		buffer.add((uint8_t*)it, sizeof(T));*/

	size_t s = 0;
	if (buffer.size()) s = stream->compress(buffer.data(), buffer.size(), out, out_offset + sizeof(size_t));
	out.resize(out_offset + sizeof(size_t) + s);
	*(size_t*)(out.data() + out_offset) = buffer.size(); // uncompressed size
	//// 
	records.remove_first_n(k);
	//LOG("out %d~",k);
}

template<typename T, typename TStream>
void GenericCompressor<T, TStream>::getIndexData (Array<uint8_t> &out) {
	out.resize(0);
	stream->getCurrentState(out);
}

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

	//fprintf(stderr,"%d %d --> ",recordCount,records.size());

	// here, we shouldn't have any leftovers, since all blocks are of the same size
	assert(recordCount == records.size());

	// decompress
	assert(in_size >= sizeof(size_t));
	
	size_t uncompressed_size = *(size_t*)in;
	Array<uint8_t> au;
	au.resize(uncompressed_size);
	in += sizeof(size_t);
	
	size_t s = 0;
	if (in_size) s = stream->decompress(in, in_size, au, 0);
	assert(s == uncompressed_size);
	assert(s % sizeof(T) == 0);
	records.resize(0);
	records.add((T*)au.data(), s / sizeof(T));

	//fprintf(stderr, "%d\n",records.size());

	recordCount = 0;
}

template<typename T, typename TStream>
void GenericDecompressor<T, TStream>::setIndexData (uint8_t *in, size_t in_size) {
	stream->setCurrentState(in, in_size);
}

#endif