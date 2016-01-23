#ifndef Stream_H
#define Stream_H

#include "../Common.h"

class CompressionStream {
protected:
	size_t compressedCount;

public:
	CompressionStream(void);
	virtual ~CompressionStream (void) {}

public:
	// virtual void compress (void *source, size_t sz, std::vector<char> &result) = 0;
	// append compressed data AFTER dest+dest_offset,
	// return new compressed size!	
	virtual size_t compress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) = 0;//uint8_t *dest, size_t dest_sz) = 0;
	virtual void getCurrentState (Array<uint8_t> &ou) {}
	virtual size_t getCount(void) { return compressedCount; }
};

class DecompressionStream {
protected:

public:
	virtual ~DecompressionStream (void) {}

public:
	// virtual void decompress (void *source, size_t sz, std::vector<char> &result) = 0;
	// append decompressed data AFTER dest+dest_offset,
	// return new decompressed size!
	virtual size_t decompress (uint8_t *source, size_t source_sz, Array<uint8_t> &dest, size_t dest_offset) = 0;//uint8_t *dest, size_t dest_sz) = 0;
	virtual void setCurrentState (uint8_t *source, size_t source_sz) {}
};

#endif
