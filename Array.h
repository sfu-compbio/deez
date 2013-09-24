#ifndef Array_H
#define Array_H

#include <cstring>
#include <inttypes.h>
#include <stdlib.h>


template<class T>
class Array {
	T 		*_records;
	size_t  _size;
	size_t  _capacity;
	size_t 	_extend;

public:
	Array (void):
		_size(0), _records(0), _capacity(0), _extend(100)
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

public:
	void realloc (size_t sz) {
		_capacity = sz;

		if (_size > _capacity) _size = _capacity;
		if (!_capacity) { _records = 0; return; }

		T *tmp = new T[_capacity];
		std::memcpy(tmp, _records, _size * sizeof(T));
		delete[] _records;
		_records = tmp;
	}

	void resize (size_t sz) {
		if (sz > _capacity)
			realloc(sz);
		_size = sz;
	}

	// add element, realloc if needed
	void add (const T &t) {
		if (_size == _capacity)
			realloc(_capacity + _extend);
		_records[_size++] = t;
	}

	// add array, realloc if needed
	void add (const T *t, size_t sz) {
		if (_size + sz > _capacity)
			realloc(_capacity + sz + _extend);
		memcpy(_records + _size, t, sz * sizeof(T));
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

	void set_extend (size_t s) {
		_extend = s;
	}
};

// typedef Array<uint8_t> ByteArray;

#endif