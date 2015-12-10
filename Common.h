#ifndef Common_H
#define Common_H

#include <assert.h>
#include <inttypes.h>
#include <typeinfo>
#include <string>

#include "DZException.h"
#include "FileIO.h"
#include "Array.h"
#include "CircularArray.h"
#include "Streams/Stream.h"

#define KB  1024LL
#define MB  KB * 1024LL
#define GB  MB * 1024LL

#define VERSION	0x11 // 0x10
#define MAGIC 	(0x07445A00 | VERSION)

double _zaman_ (void);
#define ZAMAN_START() {double _____zaman564265=_zaman_();
#define ZAMAN_END(s) /*DEBUG("[%s] %.2lf ", s, _zaman_()-_____zaman564265);*/}

//#define DZ_EVAL

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
std::string int2str (int k);
std::string inttostr (int k);
char getDNAValue (char ch);

void addEncoded (ssize_t n, Array<uint8_t> &o, uint8_t offset = 0);
ssize_t getEncoded (uint8_t *&len, uint8_t offset = 0);
std::string fullPath (const std::string &s);

#define __debug_fwrite(a,b,c,d)
// fwrite(a,b,c,d)

extern int __DC;
extern FILE **____debug_file;
extern bool optReadLossy;

#endif
