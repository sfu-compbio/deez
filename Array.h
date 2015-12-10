#ifndef Array_H
#define Array_H

#include <cstring>
#include <algorithm>
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
		_records(0), _size(0), _capacity(0), _extend(100)
	{
	}

	Array (size_t c, size_t e = 100):
		_records(0), _size(0), _capacity(c), _extend(e) 
	{
		_records = new T[_capacity];
	}

	~Array (void)
	{
		if (_records) {
			delete[] _records;
			_records = 0;
		}
	}

	Array (const Array& a)
	{
		_size = a._size;
		_capacity = a._capacity;
		_extend = a._extend;
		_records = new T[a._capacity];
		std::copy(a._records, a._records + a._size, _records);
	}

	Array(Array&& a): Array()
	{
		swap(*this, a);
	}

	Array& operator= (Array a) 
	{
		swap(*this, a);
		return *this;
	}

	friend void swap(Array& a, Array& b) // nothrow
	{
		using std::swap;

		swap(a._records, b._records);
		swap(a._size, b._size);
		swap(a._capacity, b._capacity);
		swap(a._extend, b._extend);
	}


public:
	void realloc (size_t sz) {
		_capacity = sz + _extend;

		if (_size > _capacity) _size = _capacity;
		if (!_capacity) { _records = nullptr; return; }

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

// typedef Array<uint8_t> ByteArray;

#endif