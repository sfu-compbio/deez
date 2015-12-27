#ifndef Array_H
#define Array_H

#include <cstring>
#include <algorithm>
#include <inttypes.h>
#include <stdlib.h>

#include "Utils.h"

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
	//	ZAMAN_START(Array_Copy);
	//	LOG("Copying array of size %d %d", a._size, a._capacity);
		_size = a._size;
		_capacity = a._capacity;
		_extend = a._extend;
		_records = new T[a._capacity];
		std::copy(a._records, a._records + a._size, _records);
	//	ZAMAN_END(Array_Copy);
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

public: // for range based loops
	T *begin() const
	{
		return _records;
	}

	T *end() const
	{
		return _records + _size;
	}


public:
	void realloc (size_t sz) 
	{
	//	ZAMAN_START(Realloc);
		_capacity = sz + _extend;

		if (_size > _capacity) _size = _capacity;
		if (!_capacity) { _records = nullptr; return; }

		T *tmp = new T[_capacity];
		std::copy(_records, _records + _size, tmp);
		delete[] _records;
		_records = tmp;
	//	ZAMAN_END(Realloc);
	}

	void resize (size_t sz) 
	{
		if (sz > _capacity) 
			realloc(sz);
		_size = sz;
	}

	// Add defualt element or use allocated
	void add () 
	{
		if (_size == _capacity)
			realloc(_capacity);
		_size++;
	}

	// add element, realloc if needed
	void add (const T &t) 
	{
		if (_size == _capacity)
			realloc(_capacity);
		_records[_size++] = t;
	}

	// add array, realloc if needed
	void add (const T *t, size_t sz) 
	{
		if (_size + sz > _capacity)
			realloc(_capacity + sz);
		std::copy(t, t + sz, _records + _size);
		_size += sz;
	}

	size_t size (void) const 
	{
		return _size;
	}
	
	size_t capacity (void) const 
	{
		return _capacity;
	}

	T *data (void) 
	{
		return _records;
	}

	const T *data (void) const 
	{
		return _records;
	}


	T &operator[] (size_t i) 
	{
		assert(i < _size);
		return _records[i];
	}

	const T &operator[] (size_t i) const 
	{
		assert(i < _size);
		return _records[i];
	}

	void set_extend (size_t s) 
	{
		_extend = s;
	}

	void removeFirstK (size_t k) 
	{
		ZAMAN_START(RemoveFirstK);
		if (k < _size) {
			std::copy(_records + k, _records + _size, _records);
			_size -= k;
		} else {
			_size = 0;
		}
		ZAMAN_END(RemoveFirstK);
	}

};

// typedef Array<uint8_t> ByteArray;

#endif