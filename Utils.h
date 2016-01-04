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

inline std::vector<std::string> split (std::string s, char delim) 
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> elems;
	while (std::getline(ss, item, delim)) 
		elems.push_back(item);
	return elems;
}

inline uint64_t zaman() 
{
	struct timeval t;
	gettimeofday(&t, 0);
	return (t.tv_sec * 1000000ll + t.tv_usec);
}

// #define ZAMAN
#ifdef ZAMAN
	class __zaman__ { // crazy hack for gcc 5.1
	public:
		std::map<std::string, uint64_t> times;
		std::string prefix;

	public:
		static std::map<std::string, uint64_t> times_global;
		static std::string prefix_global;
		static std::mutex mtx;
	};
	extern thread_local __zaman__ __zaman_thread__;

	#define ZAMAN_VAR(s) \
		__zaman_time_##s
	#define ZAMAN_START(s) \
		int64_t ZAMAN_VAR(s) = zaman(); \
		__zaman_thread__.prefix += std::string(#s) + "_"; 
	#define ZAMAN_END(s) \
		__zaman_thread__.times[__zaman_thread__.prefix] += (zaman() - ZAMAN_VAR(s)); \
		__zaman_thread__.prefix = __zaman_thread__.prefix.substr(0, __zaman_thread__.prefix.size() - 1 - strlen(#s)); 
	#define ZAMAN_THREAD_JOIN() \
		{ 	std::lock_guard<std::mutex> __l__(__zaman__::mtx); \
			for (auto &s: __zaman_thread__.times) \
				__zaman__::times_global[__zaman__::prefix_global + s.first] += s.second; \
			__zaman_thread__.times.clear(); \
		}
	#define ZAMAN_START_P(s) \
		int64_t ZAMAN_VAR(s) = zaman(); \
		__zaman__::prefix_global += std::string(#s) + "_"; 
	#define ZAMAN_END_P(s) \
		__zaman__::times_global[__zaman__::prefix_global] += (zaman() - ZAMAN_VAR(s)); \
		__zaman__::prefix_global = __zaman__::prefix_global.substr(0, __zaman__::prefix_global.size() - 1 - strlen(#s)); \

	inline void ZAMAN_REPORT() 
	{ 
		using namespace std;
		ZAMAN_THREAD_JOIN();
		vector<string> p;
		for (auto &tt: __zaman__::times_global) { 
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
#else
	#define ZAMAN_VAR(s)	
	#define ZAMAN_START(s) 
	#define ZAMAN_END(s)
	#define ZAMAN_THREAD_JOIN()
	#define ZAMAN_START_P(s)
	#define ZAMAN_END_P(s)
	#define ZAMAN_REPORT()
#endif

#endif // Utils_H