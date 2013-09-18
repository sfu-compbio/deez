#ifndef GenericEngine_H
#define GenericEngine_H

#include "../Common.h"
#include "Engine.h"

template<typename T, typename TStream>
class GenericCompressor: public Compressor {
protected:
	Array<T> records;

public:
	GenericCompressor (int blockSize);
	virtual ~GenericCompressor (void);

public:
	virtual void addRecord (const T &rec);
	virtual void outputRecords (Array<uint8_t> &out);
	virtual void appendRecords (Array<uint8_t> &out);

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

#include "GenericEngine.tcc"

#endif
