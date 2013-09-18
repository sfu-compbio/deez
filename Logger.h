#ifndef Logger_H
#define Logger_H

#include <string.h>
#ifdef _MSC_VER
#define FILE_NAME_EXT (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define FILE_NAME_EXT (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#define FILE_NAME	 FILE_NAME_EXT

#define SCREEN(c,...)\
	fprintf(stderr, c, ##__VA_ARGS__)
#define WARN(c,...)\
	fprintf(stderr, c"\n", ##__VA_ARGS__)
#define ERROR(c,...)\
	fprintf(stderr, c"\n", ##__VA_ARGS__)
#define LOG(c,...)\
	fprintf(stderr, c"\n", ##__VA_ARGS__)
#define DEBUG(c,...)\
	fprintf(stdout, c"\n", ##__VA_ARGS__)
#endif
