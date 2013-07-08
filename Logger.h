#ifndef Logger_H
#define Logger_H

#include <string.h>
#ifdef _MSC_VER
#define FILE_NAME_EXT (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define FILE_NAME_EXT (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#define FILE_NAME	 FILE_NAME_EXT

#define LOG(c,...)\
	fprintf(stderr, "[L %18s]  "c"\n", FILE_NAME, ##__VA_ARGS__)
#define DEBUG(c,...)\
	fprintf(stderr, "[D %18s]  "c"\n", FILE_NAME, ##__VA_ARGS__)
#define MEM_DEBUG(c,...)\
	fprintf(stderr, "[MEM %18s]  "c"\n", FILE_NAME, ##__VA_ARGS__)
#endif
