#include "Common.h"
#include <vector>
#include <string>
using namespace std;

int __DC = 0;
FILE **____debug_file;

string int2str (int k) {
	string s = "";
	while (k) {
		s = char(k % 10 + '0') + s;
		k /= 10;
	}
	return s;
}

string inttostr (int k) {
    static vector<std::string> mem;
    if (mem.size() == 0) {
    	mem.resize(10001);
		mem[0] = "0";
		for (int i = 1; i < 10000; i++)
			mem[i] = int2str(i);
    }

    if (k < 10000)
    	return mem[k];
    else
    	return int2str(k);
}

char getDNAValue (char ch) {
	static char c[128] = {
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 0 15
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 16 31
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 5, // 32 47
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 48 63
		5, 1, 5, 2, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, // 64 79
		5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 80 95
		5, 1, 5, 2, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, // 96 111
		5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 112 127
	};	
	return c[ch];
}

void addEncoded (int n, Array<uint8_t> &o) {
	if (n < (1 << 8))
		o.add(n);
	else if (n < (1 << 16)) {
		o.add(0);
		o.add((n >> 8) & 0xff);
		o.add(n & 0xff);
	}
	else {
		REPEAT(2) o.add(0);
		o.add((n >> 24) & 0xff);
		o.add((n >> 16) & 0xff);
		o.add((n >> 8) & 0xff);
		o.add(n & 0xff);	
	}
}

size_t getEncoded (uint8_t *&len) {
	int T = 1;
	if (!*len) T = 2, len++;
	if (!*len) T = 4, len++;
	size_t size = 0;
	REPEAT(T) size |= *len++ << (8 * (T-_-1));
	assert(size > 0);
	return size;
}

string S (const char* fmt, ...) {
	char *ptr = 0;
    va_list args;
    va_start(args, fmt);
    vasprintf(&ptr, fmt, args);
    va_end(args);
    string s = ptr;
    free(ptr);
    return s;
}
