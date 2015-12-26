#include "Common.h"
#include <vector>
#include <string>
#include <mutex>
using namespace std;

thread_local std::map<std::string, uint64_t> __zaman_times__;
std::map<std::string, uint64_t> __zaman_times_global__;
thread_local std::string __zaman_prefix__;
std::string __zaman_prefix_global__;
std::mutex __zaman_mtx__;

string int2str (int64_t k) 
{
	assert(k >= 0);
	string s;
	while (k) {
		s = char(k % 10 + '0') + s;
		k /= 10;
	}
	return s;
}

string inttostr (int64_t k) 
{
	static std::mutex mutex;
    static std::vector<std::string> intCache;
    if (intCache.size() == 0) {
		mutex.lock();
    	intCache.resize(10001);
		intCache[0] = "0";
		for (int64_t i = 1; i < 10000; i++)
			intCache[i] = int2str(i);
		mutex.unlock();
    }

    bool bit = 0;
    if (k < 0) bit = 1, k = -k;

    if (k < intCache.size())
    	return (bit ? "-" : "") + intCache[k];
    else
    	return (bit ? "-" : "") + int2str(k);
}

char getDNAValue (char ch) 
{
	#define _ 5
	static char c[128] = {
		_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,// 0 15
		_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,// 16 31
		_,_,_,_,_,_,_,_,_,_,_,_,_,_,0,_,// 32 47
		_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,// 48 63
		_,1,_,2,_,_,_,3,_,_,_,_,_,_,_,_,// 64 79
		_,_,_,_,4,_,_,_,_,_,_,_,_,_,_,_,// 80 95
		_,1,_,2,_,_,_,3,_,_,_,_,_,_,_,_,// 96 111
		_,_,_,_,4,_,_,_,_,_,_,_,_,_,_,_,// 112 127
	};	
	#undef _
	return c[ch];
}

void addEncoded (ssize_t n, Array<uint8_t> &o, uint8_t offset) 
{
	n += offset;
	assert(n != offset);
	if (n < (1 << 8)) {
		o.add(n);
	} else if (n < (1 << 16)) {
		o.add(offset);
		o.add((n >> 8) & 0xff);
		o.add(n & 0xff);
	} else {
		REPEAT(2) o.add(offset);
		o.add((n >> 24) & 0xff);
		o.add((n >> 16) & 0xff);
		o.add((n >> 8) & 0xff);
		o.add(n & 0xff);	
	}
}

ssize_t getEncoded (uint8_t *&len, uint8_t offset) 
{
	int T = 1;
	if (*len == offset) T = 2, len++;
	if (*len == offset) T = 4, len++;
	ssize_t size = 0;
	REPEAT(T) size |= *len++ << (8 * (T-_-1));
	assert(size != offset);
	size -= offset;
	return size;
}

int packInteger(uint64_t num, Array<uint8_t> &o) 
{
	if (num < (1 << 8)) {
		o.add(num & 0xff);
		return 0;
	} else if (num < (1 << 16)) {
		REPEAT(2) o.add(num & 0xff), num >>= 8;
		return 1;
	} else if (num < (1 << 24)) {
		REPEAT(3) o.add(num & 0xff), num >>= 8;
		return 2;
	} else if (num < (1ll << 32)) {
		REPEAT(4) o.add(num & 0xff), num >>= 8;
		return 3;
	} else {
		REPEAT(8) o.add(num & 0xff), num >>= 8;
		return 4;
	}
}

uint64_t unpackInteger(int T, Array<uint8_t> &i, size_t &ii) 
{
	uint64_t e = 0;
	if (T == 4) T += 3;
	REPEAT(T + 1) e |= (uint64_t)(i.data()[ii++]) << (8 * _);
	return e;
}

string S (const char* fmt, ...) 
{
	char *ptr = 0;
    va_list args;
    va_start(args, fmt);
    vasprintf(&ptr, fmt, args);
    va_end(args);
    string s = ptr;
    free(ptr);
    return s;
}

template<>
size_t sizeInMemory(std::string t) {
	size_t sz = 0;
	for (int i = 0; i < t.capacity(); i++)
		sz += sizeInMemory(*(&t[0] + i));
	return sizeof(t) + t.capacity();
}

