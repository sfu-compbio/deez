#ifndef Common_H
#define Common_H

#include <assert.h>
#include <inttypes.h>
#include <typeinfo>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <sstream>
#include <sys/time.h>

#include "ctpl.h"
#include "Utils.h"
#include "DZException.h"
#include "FileIO.h"
#include "Array.h"
#include "CircularArray.h"
#include "Streams/Stream.h"

extern ctpl::thread_pool ThreadPool;
extern bool optStdout;
extern int  optThreads;
extern int  optLogLevel;

std::string S (const char* fmt, ...);
void initCache();
void inttostr (int64_t k, FILE *f);
void inttostr (int64_t k, std::string &r);
std::string xtoa (int64_t k);

char getDNAValue (char ch);

void addEncoded (size_t n, Array<uint8_t> &o, uint8_t offset = 0);
ssize_t getEncoded (uint8_t *&len, uint8_t offset = 0);
int packInteger(uint64_t num, Array<uint8_t> &o);
uint64_t unpackInteger(int T, Array<uint8_t> &i, size_t &ii) ;

extern bool optReadLossy;	
extern bool optBzip;

template<typename T>
size_t sizeInMemory(T t) {
	return sizeof(t);
}
template<typename T>
size_t sizeInMemory(Array<T> t) {
	size_t sz = 0;
	for (int i = 0; i < t.capacity(); i++)
		sz += sizeInMemory(t.data()[i]);
	return sizeof(T) + sz;
}
template<typename T>
size_t sizeInMemory(std::vector<T> t) {
	size_t sz = 0;
	for (int i = 0; i < t.capacity(); i++)
		sz += sizeInMemory(*(&t[0] + i));
	return sizeof(T) + sz;
}
// template<typename K, typename V>
// size_t sizeInMemory(std::map<K, V> t) {
// 	return (sizeof(K) + sizeof(V) + sizeof(std::_Rb_tree_node_base) ) * t.size() + 
// 	sizeof(std::_Rb_tree<K,V,K,std::less<K>, std::allocator<std::pair<const K, V>>>);
// }
template<>
size_t sizeInMemory(std::string t);

#endif
