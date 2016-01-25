#pragma once

#include <assert.h>
#include <stdarg.h>
#include <inttypes.h>
#include <typeinfo>
#include <string>
#include <exception>

#define KB  1024LL
#define MB  KB * 1024LL
#define GB  MB * 1024LL

// #define VERSION	0x11 // 0x10
// #define MAGIC 	(0x07445A00 | VERSION)

extern bool optStdout;
extern int  optThreads;
extern int  optLogLevel;

#define WARN(c,...)\
	fprintf(stderr, c"\n", ##__VA_ARGS__)
#define ERROR(c,...)\
	fprintf(stderr, c"\n", ##__VA_ARGS__)
#define LOG(c,...)\
	{if(optLogLevel>=1) fprintf(stderr, c"\n", ##__VA_ARGS__);}
#define DEBUG(c,...)\
	{if(optLogLevel>=2) fprintf(stderr, c"\n", ##__VA_ARGS__);}
#define DEBUGN(c,...)\
	{if(optLogLevel>=2) fprintf(stderr, c, ##__VA_ARGS__);}
#define LOGN(c,...)\
	{if(optLogLevel>=1) fprintf(stderr, c, ##__VA_ARGS__);}

#define REPEAT(x)\
	for(int _=0;_<x;_++)
#define foreach(i,c) \
	for (auto i = (c).begin(); i != (c).end(); ++i)

namespace Legacy {
namespace v11 {

class DZException: public std::exception {
protected:
	char msg[256];

public:
	DZException (void) {}
	DZException (const char *s, ...) {
		va_list args;
		va_start(args, s);
		vsprintf(msg, s, args);
		va_end(args);
	}

	const char *what (void) const throw() {
		return msg;
	}
};

class DZSortedException : public DZException {
public:
	DZSortedException (const char *s, ...) {
		va_list args;
		va_start(args, s);
		vsprintf(msg, s, args);
		va_end(args);
	}
};

template<class T> 
class Array {
	T 		*_records;
	size_t  _size;
	size_t  _capacity;
	size_t 	_extend;

public:
	Array (void):
		_records(0), _size(0), _capacity(0), _extend(100)
	{
	}

	Array (size_t c, size_t e = 100):
		_size(0), _capacity(c), _extend(e) 
	{
		_records = new T[_capacity];
	}

	~Array (void) {
		delete[] _records;
	}

	Array (const Array& a)
	{
		_size = a._size;
		_capacity = a._capacity;
		_extend = a._extend;
		_records = new T[a._capacity];
		std::copy(a._records, a._records + a._size, _records);
	}

	Array& operator= (const Array& a) 
	{
		_size = a._size;
		_capacity = a._capacity;
		_extend = a._extend;
		_records = new T[a._capacity];
		std::copy(a._records, a._records + a._size, _records);	
		return *this;
	}

public:
	void realloc (size_t sz) {
		_capacity = sz + _extend;

		if (_size > _capacity) _size = _capacity;
		if (!_capacity) { _records = 0; return; }

		T *tmp = new T[_capacity];
		std::copy(_records, _records + _size, tmp);
		delete[] _records;
		_records = tmp;
	}

	void resize (size_t sz) {
		if (sz > _capacity) {
			realloc(sz);
		}
		_size = sz;
	}

	// add element, realloc if needed
	void add (const T &t) {
		if (_size == _capacity)
			realloc(_capacity);
		_records[_size++] = t;
	}

	// add array, realloc if needed
	void add (const T *t, size_t sz) {
		if (_size + sz > _capacity)
			realloc(_capacity + sz);
		std::copy(t, t + sz, _records + _size);
		_size += sz;
	}

	size_t size (void) const {
		return _size;
	}
	
	size_t capacity (void) const {
		return _capacity;
	}

	T *data (void) {
		return _records;
	}

	T &operator[] (size_t i) {
		assert(i < _size);
		return _records[i];
	}

	void set_extend (size_t s) {
		_extend = s;
	}
};

std::string S (const char* fmt, ...);
std::string int2str (int k);
std::string inttostr (int k);
char getDNAValue (char ch);

void addEncoded (int n, Array<uint8_t> &o);
size_t getEncoded (uint8_t *&len);
uint64_t decodeUInt (int T, Array<uint8_t> &content, size_t &cc);

};
};