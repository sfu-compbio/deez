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
#include "DZException.h"
#include "FileIO.h"
#include "Array.h"
#include "CircularArray.h"
#include "Streams/Stream.h"

#define KB  1024LL
#define MB  KB * 1024LL
#define GB  MB * 1024LL

#define VERSION	0x12 // 0x10
#define MAGIC 	(0x07445A00 | VERSION)

//#define DZ_EVAL


extern ctpl::thread_pool ThreadPool;
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

std::string S (const char* fmt, ...);
std::string int2str (int64_t k);
std::string inttostr (int64_t k);
char getDNAValue (char ch);

void addEncoded (ssize_t n, Array<uint8_t> &o, uint8_t offset = 0);
ssize_t getEncoded (uint8_t *&len, uint8_t offset = 0);
int packInteger(uint64_t num, Array<uint8_t> &o);
uint64_t unpackInteger(int T, Array<uint8_t> &i, size_t &ii) ;

extern bool optReadLossy;

inline uint64_t zaman() 
{
	struct timeval t;
	gettimeofday(&t, 0);
	return (t.tv_sec * 1000000ll + t.tv_usec);
}

extern std::map<std::string, uint64_t> __ts__times__;
#define ZAMAN_VAR(s) __ts__##s
#define ZAMAN_START(s) int64_t ZAMAN_VAR(s) = zaman();
#define ZAMAN_END(s) __ts__times__[ #s ] += (zaman() - ZAMAN_VAR(s));
inline std::vector<std::string> split (std::string s, char delim) 
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> elems;
	while (std::getline(ss, item, delim)) 
		elems.push_back(item);
	return elems;
}
inline void ZAMAN_REPORT() 
{ 
	using namespace std;
	vector<string> p;
	for (auto &tt: __ts__times__) { 
		string s = tt.first; 
		auto f = split(s, '_');
		s = "";
		for (int i = 0; i < f.size(); i++) {
			if (i < p.size() && p[i] == f[i])
				s += "  ";
			else
				s += "/" + f[i];
		}
		p = f;
		LOG("  %-40s: %'8.1lfs", s.c_str(), tt.second/1000000.0);
	}
}


#endif
