#ifndef Stream_H
#define Stream_H

#include <vector>

class CompressionStream {
protected:

public:
	virtual ~CompressionStream (void) {}

public:
	virtual void compress (void *source, size_t sz, std::vector<char> &result) = 0;
};

class DecompressionStream {
protected:

public:
	virtual ~DecompressionStream (void) {}

public:
	virtual void decompress (void *source, size_t sz, std::vector<char> &result) = 0;
};

#endif
