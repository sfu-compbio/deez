#ifndef Utils_H
#define Utils_H

#define KB  1024LL
#define MB  KB * 1024LL
#define GB  MB * 1024LL

#define VERSION	0x12 // 0x10
#define MAGIC 	(0x07445A00 | VERSION)

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

inline uint64_t zaman() 
{
	struct timeval t;
	gettimeofday(&t, 0);
	return (t.tv_sec * 1000000ll + t.tv_usec);
}

extern thread_local std::map<std::string, uint64_t> __zaman_times__;
extern std::map<std::string, uint64_t> __zaman_times_global__;
extern thread_local std::string __zaman_prefix__;
extern std::string __zaman_prefix_global__;
extern std::mutex __zaman_mtx__;
#define ZAMAN_VAR(s) \
	__zaman_time_##s
#define ZAMAN_START(s) \
	int64_t ZAMAN_VAR(s) = zaman(); \
	__zaman_prefix__ += std::string(#s) + "_"; 
#define ZAMAN_END(s) \
	__zaman_times__[__zaman_prefix__] += (zaman() - ZAMAN_VAR(s)); \
	__zaman_prefix__ = __zaman_prefix__.substr(0, __zaman_prefix__.size() - 1 - strlen(#s)); 
#define ZAMAN_THREAD_JOIN() \
	{ 	std::lock_guard<std::mutex> __l__(__zaman_mtx__); \
		for (auto &s: __zaman_times__) \
			__zaman_times_global__[__zaman_prefix_global__ + s.first] += s.second; \
	}
#define ZAMAN_START_P(s) \
	int64_t ZAMAN_VAR(s) = zaman(); \
	__zaman_prefix_global__ += std::string(#s) + "_"; 
#define ZAMAN_END_P(s) \
	__zaman_times_global__[__zaman_prefix_global__] += (zaman() - ZAMAN_VAR(s)); \
	__zaman_prefix_global__ = __zaman_prefix_global__.substr(0, __zaman_prefix_global__.size() - 1 - strlen(#s)); \


#endif // Utils_H