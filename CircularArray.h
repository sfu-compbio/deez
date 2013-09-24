#ifndef CircularArray_H
#define CircularArray_H

#include <cstring>
#include <inttypes.h>
#include <stdlib.h>

template<class T>
class CircularArray {
	T 		*_records;

	size_t  _size;
	size_t  _start;

	size_t  _capacity;

public:
	CircularArray (size_t cap):
		_size(0), _start(0), _capacity(cap)
	{
		_records = new T[_capacity];
	}

	~CircularArray (void) {
		delete[] _records;
	}

public:
	void resize (size_t sz) {
		assert(sz <= _capacity);
		_size = sz;
	}

	// add element, realloc if needed
	void add (const T &t) {
		assert(_size < _capacity);
		_records[(_start + _size) % _capacity] = t;
		_size++;
	}

	// add array, realloc if needed
	void add (const T *t, size_t sz) {
		assert(_size + sz <= _capacity);
		for (size_t i = 0; i < sz; i++)
			add(t[i]);
	}

	T &operator[] (size_t i) {
		assert(i < _size);
		return _records[(_start + i) % _capacity];
	}

	size_t size (void) const {
		return _size;
	}

	void remove_first_n (size_t k) {
		assert (k <= _size);
		_size -= k;
		_start = (_start + k) % _capacity;
	}
};

// typedef Array<uint8_t> ByteArray;

#endif